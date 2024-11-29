#include "ImageCache.h"

using namespace ImageCaching;


template<typename TImage>
void ImageCache<TImage>::SetMaxMemory(int64_t maximumMemoryInBytes)
{
	if (maximumMemoryInBytes < 0)
		throw std::runtime_error("Max memory must be positive");

	_maxAllowedMemory = maximumMemoryInBytes;
	//TODO: zoea 25/11/2024 remove items if needed when the memory cap is reduced.
}


template<typename TImage>
void ImageCache<TImage>::CheckMemoryUsage()
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

template<typename TImage>
TryGetImageResult ImageCache<TImage>::TryGetImage(const std::filesystem::path& imagePath, const TImage* outImage)
{
	std::lock_guard<std::recursive_mutex> lockGuard(_cacheLock);

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

template<typename TImage>
TryGetImageResult ImageCache<TImage>::TryGetImageAtSize(
	const std::filesystem::path& imagePath,
	unsigned int width, 
	unsigned int height, 
	const TImage*& outImage) 
{
	std::lock_guard<std::recursive_mutex> lockGuard(_cacheLock);

	//Check if the image is in the cache at it's source size
	const auto key = imagePath.string();
	if (auto search = _images.find(key); search != _images.end())
	{
		ImageCacheEntry* cacheEntry = search->second;
		const auto* sourceImageItem = cacheEntry->SourceImageItem.Image;
		/*
		if (sourceImageItem->GetWidth() == width && sourceImageItem->GetHeight() == height)
		{
			//The requested size is the source size.
			outImage = cacheEntry->SourceImageItem.Image;
			return TryGetImageResult::FoundExactMatch;
		}*/

		const auto& resizedImages = cacheEntry->ResizedImages;
		const auto resizedImageKey = ResizedImageKey(width, height).ToString();
		if (auto resizedSearch = resizedImages.find(resizedImageKey); resizedSearch != resizedImages.end())
		{
			outImage = resizedSearch->second->Image;
			return TryGetImageResult::FoundExactMatch;
		}

		//Return the source image if there was no cached copy at the specified dimensions
		outImage = cacheEntry->SourceImageItem.Image;
		return TryGetImageResult::FoundSourceImageOfDifferentDimensions;
	}

	outImage = nullptr;
	return TryGetImageResult::NotFound;
}

template<typename TImage>
TryAddImageResult ImageCache<TImage>::TryAddSourceImage(const IImageSource* image)
{
	if (!image)
		return TryAddImageResult::NoChange;

	std::lock_guard<std::recursive_mutex> lockGuard(_cacheLock);

	const auto key = image->GetImagePath().string();

	if (auto search = _images.find(key); search != _images.end())
	{
		//TODO: zoea 28/11/2024 implement an affordance to inform the caller that a source image already existed in the cache, but was
		//not the same instance of source data. Facilitating either updating the source image and all instances at each size, or to delete the duplicate 
		//load
		return TryAddImageResult::NoChange;
	}

	_images[key] = new ImageCacheEntry<TImage>(image);
	_currentMemoryUsage += image->GetSizeInBytes();

}

template<typename TImage>
TryAddImageResult ImageCache<TImage>::TryAddImage(const TImage* image)
{
	
	std::lock_guard<std::recursive_mutex> lockGuard(_cacheLock);

	const auto key = image->GetImagePath().string();

	std::shared_ptr<const IImage> sharedPtr = make_shared_with_callback(image);

	if (auto search = _images.find(key); search != _images.end())
	{
		ImageCacheEntry<TImage>* cacheEntry = search->second;
		auto& sourceImageItem = cacheEntry->SourceImageItem;

		//update last accessed time of the source image, even if we update a resized version because the source should not be
		//expired if a resized copy is more recently used.
		sourceImageItem.UpdateLastAccessedTime();

		const auto* sourceImage = sourceImageItem.Image;

		if (image == sourceImage) //is the exact same instance.
			return TryAddImageResult::NoChange;

		if (image->GetWidth() == sourceImage->GetWidth() &&
			image->GetHeight() == sourceImage->GetHeight())
		{
			//not a different size than source.
			return TryAddImageResult::NoChange;
		}

		const auto resizedImageKey = ResizedImageKey(image->GetWidth(), image->GetHeight()).ToString();

		//If there are copies of the image at different sizes, check for a match.
		if (cacheEntry->ResizedImages)
		{
			const auto& resizedImages = cacheEntry->ResizedImages;
			if (auto resizedSearch = resizedImages.find(resizedImageKey); resizedSearch != resizedImages.end())
			{
				auto* resizedImageItem = resizedSearch->second;
				resizedImageItem->UpdateLastAccessedTime();
				if( resizedImageItem->Image == image )
					return TryAddImageResult::NoChange;

				if (resizedImageItem->Image->GetWidth() == image->GetWidth() && 
					resizedImageItem->Image->GetHeight() == image->GetHeight())
					return TryAddImageResult::NoChange;
			}
		}

		//image is a resized version not in the cache, add it.

		//ensure the resized images map exists.
		if (!cacheEntry->ResizedImages)
		{
			cacheEntry->ResizedImages = new std::map<const std::string, ImageCacheItem<TImage>*>();
		}


		(*cacheEntry->ResizedImages)[resizedImageKey] = new ImageCacheItem<TImage>(image);
		_currentMemoryUsage += image->GetSizeInBytes();
		CheckMemoryUsage();
		return TryAddImageResult::AddedAsResizedImage;
	}

	_images[key] = new ImageCacheEntry<TImage>(image);
	_currentMemoryUsage += image->GetSizeInBytes();

	CheckMemoryUsage();
	return TryAddImageResult::Added;
}

template<typename TImage>
bool ImageCaching::ImageCache<TImage>::TryRemoveImage(const TImage* image)
{
	std::lock_guard<std::recursive_mutex> lockGuard(_cacheLock);
	const auto key = image->GetImagePath().string();
	bool removed = false;

	if (auto search = _images.find(key); search != _images.end())
	{
		ImageCacheEntry* cacheEntry = search->second;
		const auto resizedImageKey = ResizedImageKey(image->GetWidth(), image->GetHeight()).ToString();
		const auto removedCount = cacheEntry->ResizedImages.erase(resizedImageKey);
		removed = removedCount > 0;

		if (cacheEntry->ResizedImages.empty())
		{
			_images.erase(key);
			delete ImageCacheEntry;
		}
	} 
}
