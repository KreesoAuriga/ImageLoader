#include "ImageCache.h"

using namespace ImageCaching;


void ImageCache::SetMaxMemory(int64_t maximumMemoryInBytes)
{
	if (maximumMemoryInBytes < 0)
		throw std::runtime_error("Max memory must be positive");

	_maxAllowedMemory = maximumMemoryInBytes;
	//TODO: zoea 25/11/2024 remove items if needed when the memory cap is reduced.
}


void ImageCache::CheckMemoryUsage()
{
	while (_currentMemoryUsage > _maxAllowedMemory)
	{
		std::time_t earliestAccessTime = std::numeric_limits<time_t>::max();
		ImageCacheEntry* earliestCacheEntry = nullptr;

		for (auto entry : _images)
		{
			const auto entryTime = entry.second->SourceImageItem.GetLastAccessedTime();
			if (entryTime < earliestAccessTime)
			{
				earliestAccessTime = entryTime;
				earliestCacheEntry = entry.second;
			}
		}

		if (!earliestCacheEntry)
			break;

		const auto sizeOfEarliestItem = earliestCacheEntry->GetTotalSizeInBytes();
		_currentMemoryUsage -= sizeOfEarliestItem;

		_images.erase(earliestCacheEntry->ImagePath.string());
		delete earliestCacheEntry;
	}
}

TryGetImageResult ImageCache::TryGetImage(const std::filesystem::path& imagePath, const IImage* outImage)
{
	std::lock_guard<std::mutex> lockGuard(_cacheLock);

	const auto key = imagePath.string();
	if (auto search = _images.find(key); search != _images.end())
	{
		ImageCacheEntry* cacheEntry = search->second;
		outImage = cacheEntry->SourceImageItem.Image;
		cacheEntry->SourceImageItem.UpdateLastAccessedTime();
		return TryGetImageResult::FoundExactMatch;
	}

	outImage = nullptr;
	return TryGetImageResult::NotFound;
}

TryGetImageResult ImageCache::TryGetImageAtSize(
	const std::filesystem::path& imagePath,
	unsigned int width, 
	unsigned int height, 
	const IImage*& outImage) 
{
	std::lock_guard<std::mutex> lockGuard(_cacheLock);

	//Check if the image is in the cache at it's source size
	const auto key = imagePath.string();
	if (auto search = _images.find(key); search != _images.end())
	{
		ImageCacheEntry* cacheEntry = search->second;
		const auto* sourceImageItem = cacheEntry->SourceImageItem.Image;

		//update last accessed time of the source image, even if we return a resized version because the source should not be
		//expired if a resized copy is more recently used.
		cacheEntry->SourceImageItem.UpdateLastAccessedTime();
		if (sourceImageItem->GetWidth() == width && sourceImageItem->GetHeight() == height)
		{
			//The requested size is the source size.
			outImage = cacheEntry->SourceImageItem.Image;
			return TryGetImageResult::FoundExactMatch;
		}

		//If there copies of the image at different sizes, check for a match.
		if (cacheEntry->ResizedImages)
		{
			const auto& resizedImages = *cacheEntry->ResizedImages;
			const auto resizedImageKey = ResizedImageKey(width, height).ToString();
			if (auto resizedSearch = resizedImages.find(resizedImageKey); resizedSearch != resizedImages.end())
			{
				outImage = resizedSearch->second->Image;
				resizedSearch->second->UpdateLastAccessedTime();
				return TryGetImageResult::FoundExactMatch;
			}
		}

		//Return the source image if there was no cached copy at the specified dimensions
		outImage = cacheEntry->SourceImageItem.Image;
		return TryGetImageResult::FoundSourceImageOfDifferentDimensions;
	}

	outImage = nullptr;
	return TryGetImageResult::NotFound;
}

AddOrUpdateImageResult ImageCache::AddOrUpdateImage(const IImage* image)
{
	
	std::lock_guard<std::mutex> lockGuard(_cacheLock);

	const auto key = image->GetImagePath().string();
	if (auto search = _images.find(key); search != _images.end())
	{
		ImageCacheEntry* cacheEntry = search->second;
		auto& sourceImageItem = cacheEntry->SourceImageItem;

		//update last accessed time of the source image, even if we update a resized version because the source should not be
		//expired if a resized copy is more recently used.
		sourceImageItem.UpdateLastAccessedTime();

		const auto* sourceImage = sourceImageItem.Image;

		if (image == sourceImage)
			return AddOrUpdateImageResult::NoChange;

		if (image->GetWidth() == sourceImage->GetWidth() &&
			image->GetHeight() == sourceImage->GetHeight())
		{
			//cached item and provided item are not the same, but are the same size. Replace the cached item.
			cacheEntry->SourceImageItem = image;
			return AddOrUpdateImageResult::Updated;
		}

		const auto resizedImageKey = ResizedImageKey(image->GetWidth(), image->GetHeight()).ToString();

		//If there are copies of the image at different sizes, check for a match.
		if (cacheEntry->ResizedImages)
		{
			const auto& resizedImages = *cacheEntry->ResizedImages;
			if (auto resizedSearch = resizedImages.find(resizedImageKey); resizedSearch != resizedImages.end())
			{
				auto* resizedImageItem = resizedSearch->second;
				resizedImageItem->UpdateLastAccessedTime();
				if( resizedImageItem->Image == image)
					return AddOrUpdateImageResult::NoChange;

				resizedImageItem->Image = image;
				return AddOrUpdateImageResult::UpdatedAsResizedImage;
			}
		}

		//image is a resized version not in the cache, add it.

		//ensure the resized images map exists.
		if (!cacheEntry->ResizedImages)
		{
			cacheEntry->ResizedImages = new std::map<const std::string, ImageCacheItem*>();
		}


		(*cacheEntry->ResizedImages)[resizedImageKey] = new ImageCacheItem(image);
		_currentMemoryUsage += image->GetSizeInBytes();
		CheckMemoryUsage();
		return AddOrUpdateImageResult::AddedAsResizedImage;
	}

	_images[key] = new ImageCacheEntry(image);
	_currentMemoryUsage += image->GetSizeInBytes();

	CheckMemoryUsage();
	return AddOrUpdateImageResult::Added;
}

bool ImageCaching::ImageCache::TryRemoveImage(const IImage* image)
{
	return false;
}
