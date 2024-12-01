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
ImageCaching::TryGetImageResult ImageCache<TImage>::TryGetImage(
	const std::filesystem::path& imagePath,
	std::shared_ptr<const TImage>& outImage,
	const IImageSource*& outSourceImage)
{
	using namespace ImageCaching;
	std::lock_guard<std::recursive_mutex> lockGuard(_cacheLock);

	const auto key = imagePath.string();
	if (auto search = _images.find(key); search != _images.end())
	{
		ImageCacheEntry<TImage>* cacheEntry = search->second;
		outSourceImage = cacheEntry->SourceImage;

		ImageCacheItem<TImage>* resized = cacheEntry->TryGetResizedImageCacheItem(outImage->GetWidth(), outImage->GetHeight());
		if (resized)
		{
			outImage = resized->GetImage();
			return TryGetImageResult::FoundExactMatch;
		}

		return TryGetImageResult::FoundSourceImageOfDifferentDimensions;
	}

	outImage = nullptr;
	return TryGetImageResult::NotFound;
}

template<typename TImage>
ImageCaching::TryGetImageResult ImageCache<TImage>::TryGetImageAtSize(
	const std::filesystem::path& imagePath,
	unsigned int width, 
	unsigned int height,
	std::shared_ptr<const TImage>& outImage,
	const IImageSource*& outSourceImage)
{
	using namespace ImageCaching;
	std::lock_guard<std::recursive_mutex> lockGuard(_cacheLock);

	//Check if the image is in the cache at it's source size
	const auto key = imagePath.string();
	if (auto search = _images.find(key); search != _images.end())
	{
		ImageCacheEntry<TImage>* cacheEntry = search->second;
		outSourceImage = cacheEntry->SourceImage;

		auto* resized = cacheEntry->TryGetResizedImageCacheItem(width, height);
		if (resized)
		{
			outImage = resized->GetImage();
			return TryGetImageResult::FoundExactMatch;
		}

		return TryGetImageResult::FoundSourceImageOfDifferentDimensions;
	}

	outImage = nullptr;
	return TryGetImageResult::NotFound;
}

template<typename TImage>
ImageCaching::TryAddImageResult ImageCache<TImage>::TryAddSourceImage(const IImageSource* image)
{
	using namespace ImageCaching;

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

	const auto imageSize = image->GetSizeInBytes();
	if (_currentMemoryUsage + imageSize > _maxAllowedMemory)
	{
		return TryAddImageResult::OutOfMemory;
	}

	_currentMemoryUsage += imageSize;
	auto* entry = new ImageCacheEntry<TImage>(image);;
	_images[key] = entry;
	return TryAddImageResult::Added;
}

template<typename TImage>
ImageCaching::TryAddImageResult ImageCache<TImage>::TryAddImage(std::shared_ptr<const TImage> image, const TImage*& outImage)
{
	using namespace ImageCaching;
	
	std::lock_guard<std::recursive_mutex> lockGuard(_cacheLock);

	outImage = nullptr;
	const auto key = image->GetImagePath().string();


	if (auto search = _images.find(key); search != _images.end())
	{
		ImageCacheEntry<TImage>* cacheEntry = search->second;
		const IImageSource* sourceImage = cacheEntry->SourceImage;


		const auto resizedImageKey = ResizedImageKey(image->GetWidth(), image->GetHeight()).ToStringKey();

		//If there are copies of the image at different sizes, check for a match.
		const auto& resizedImages = cacheEntry->ResizedImages;
		if (auto resizedSearch = resizedImages.find(resizedImageKey); resizedSearch != resizedImages.end())
		{
			auto* resizedImageItem = resizedSearch->second;
			const auto* resizedImage = resizedImageItem->GetImage().get();

			if (resizedImage == image.get())
			{
				outImage = image.get();
				return TryAddImageResult::NoChange;
			}

			if (resizedImage->GetWidth() == image.get()->GetWidth() &&
				resizedImage->GetHeight() == image.get()->GetHeight())
			{
				outImage = image.get();
				return TryAddImageResult::NoChange;
			}
		}

		const auto imageSize = image->GetSizeInBytes();
		if (_currentMemoryUsage + imageSize > _maxAllowedMemory)
		{
			return TryAddImageResult::OutOfMemory;
		}

		//image is a resized version not in the cache, add it.
		std::weak_ptr<const TImage> weakPtr = image;
		cacheEntry->ResizedImages[resizedImageKey] = new ImageCacheItem<TImage>(weakPtr);
		_currentMemoryUsage += image->GetSizeInBytes();

		return TryAddImageResult::AddedAsResizedImage;
	}

	throw std::runtime_error("Cannot add image without first adding its imageSource");
	return TryAddImageResult::NoChange;
}

template<typename TImage>
bool ImageCache<TImage>::TryRemoveImage(const TImage* image)
{
	std::lock_guard<std::recursive_mutex> lockGuard(_cacheLock);
	const auto key = image->GetImagePath().string();
	bool removed = false;

	if (auto search = _images.find(key); search != _images.end())
	{
		ImageCacheEntry<TImage>* cacheEntry = search->second;
		const auto resizedImageKey = ResizedImageKey(image->GetWidth(), image->GetHeight()).ToStringKey();
		const auto removedCount = cacheEntry->ResizedImages.erase(resizedImageKey);
		_currentMemoryUsage -= image->GetSizeInBytes() * removedCount;
		removed = removedCount > 0;

		if (cacheEntry->ResizedImages.empty())
		{
			_currentMemoryUsage -= cacheEntry->SourceImage->GetSizeInBytes();
			_images.erase(key);
			delete cacheEntry;
		}
	} 

	return removed;
}
