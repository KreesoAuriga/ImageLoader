#pragma once
#include "../ImageCache.h"
#include "../Image.h"
#include <string>
#include <map>
#include <filesystem>
#include <mutex>
#include <memory>

struct ResizedImageKey
{
	const int Width;
	const int Height;

	ResizedImageKey(const int width, const int height)
		: Width(width)
		, Height(height)
	{
	}

	std::string ToStringKey() const
	{
		std::string result = std::to_string(Width) + ":" + std::to_string(Height);
		return result;
	}
};

template<typename TImage>
class ImageCacheItem
{

	std::weak_ptr<const TImage> _image;

public:

	ImageCacheItem(std::weak_ptr<const TImage>& image)
		: _image(image)
	{
	}

	[[nodiscard]]
	std::shared_ptr<const TImage> GetImage()
	{
		return _image.lock();
	}

};

template<typename TImage>
struct ImageCacheEntry final
{
	const std::filesystem::path ImagePath;
	const IImageSource* SourceImage;

	std::map<const std::string, ImageCacheItem<TImage>*> ResizedImages;

	ImageCacheEntry(const IImageSource* sourceImage)
		: ImagePath(sourceImage->GetImagePath())
		, SourceImage(sourceImage)
	{
		static_assert(std::is_convertible_v<TImage*, IImage*>, "TImage type must inherit from IImage.");
	}

	~ImageCacheEntry()
	{
		for (const auto& entry : ResizedImages)
			delete entry.second;

		ResizedImages.clear();

		delete SourceImage;
	}

	ImageCacheItem<TImage>* TryGetResizedImageCacheItem(const int width, const int height)
	{
		const auto resizedImageKey = ResizedImageKey(width, height).ToStringKey();
		if (auto resizedSearch = ResizedImages.find(resizedImageKey); resizedSearch != ResizedImages.end())
		{
			return resizedSearch->second;
		}

		return nullptr;
	}

	unsigned int GetTotalSizeInBytes()
	{
		unsigned int result = SourceImage->GetSizeInBytes();

		if (ResizedImages)
		{
			for (auto resizedEntry : *ResizedImages)
			{
				result += resizedEntry.second->Image->GetSizeInBytes();
			}
		}

		return result;
	}
};

/// <summary>
/// Default implementation of <see cref="IImageCache"/>. This implementation is not threadsafe, and thread safety must be implemented
/// by 
/// </summary>
template<typename TImage>
class ImageCache final : public ImageCaching::IImageCache<TImage>
{
	int64_t _maxAllowedMemory;
	int64_t _currentMemoryUsage = 0;
	std::recursive_mutex _cacheLock;
	std::map<const std::string, ImageCacheEntry<TImage>*> _images;

public:
	ImageCache(const int64_t maximumMemoryInBytes)
	{
		static_assert(std::is_convertible_v< TImage*, IImage*>, "TImage type must inherit from IImage.");
		SetMaxMemory(maximumMemoryInBytes);
	}

	size_t GetCacheEntryCount()
	{
		std::lock_guard<std::recursive_mutex> lockGuard(_cacheLock);
		return _images.size();
	}

	int64_t GetCurrentMemoryUsage() const
	{
		return _currentMemoryUsage;
	}

	/// <summary>
	/// Sets the maximum memory in bytes that the cache is allowed to use. 
	/// </summary>
	/// <param name="maximumMemoryInBytes"></param>
	virtual void SetMaxMemory(int64_t maximumMemoryInBytes) override;

	/// <summary>
	/// Gets the maximum memory in bytes that the cache is allowed to use.
	/// </summary>
	virtual int64_t GetMaxMemory() const override {
		return _maxAllowedMemory;
	}

	/// <summary>
	/// Attempts to get the image identified by the specified path.
	/// </summary>
	/// <param name="imagePath">Source path of the image.</param>
	/// <param name="outImage">The image instance if type TImage retrieved from the cache or loaded from the path.</param>
	/// <param name="outSourceImage">The source image instance retrieved from the cache or loaded from the path.</param>
	/// <returns>Result of the operation</returns>
	virtual ImageCaching::TryGetImageResult TryGetImage(const std::filesystem::path& imagePath,
		std::shared_ptr<const TImage>& outImage,
		const IImageSource*& outSourceImage) override;

	/// <summary>
	/// Attempts to get the image identified by the specified path, with the specified width and height in pixels.
	/// </summary>
	/// <param name="imagePath">Source path of the image.</param>
	/// <param name="width">The width in pixels of the image to be retrieved.</returns>
	/// <param name="height">The height in pixels of the image to be retrieved.</returns>
	/// <param name="outImage">The image instance if type TImage retrieved from the cache or loaded from the path.</param>
	/// <param name="outSourceImage">The source image instance retrieved from the cache or loaded from the path.</param>
	/// <returns>Result of the operation</returns>
	virtual ImageCaching::TryGetImageResult TryGetImageAtSize(const std::filesystem::path& imagePath,
		unsigned int width, unsigned int height,
		std::shared_ptr<const TImage>& outImage,
		const IImageSource*& outSourceImage) override;

	/// <summary>
	/// Adds the image to the cache, unless the image already exists in the cache at the image's path.
	/// If there is already an image in the cache for the image's path, outImage will be the instance that that was in the cache.
	/// </summary>
	/// <param name="image">The image to add into the cache.</param>
	/// <param name="outImage">If a different instance already exists in the cache at the image's path, this will be set to that instance.
	/// Otherwise is nullptr.</param>
	/// <returns>Result of the operation</returns>
	virtual ImageCaching::TryAddImageResult TryAddImage(std::shared_ptr<const TImage> image, const TImage*& outImage) override;

	/// <summary>
	/// Adds the source image containing the pixel data of the source file to the cache, unless the source image already 
	/// exists in the cache.
	/// </summary>
	/// <param name="image">The source image</param>
	/// <returns>Result of the operation</returns>
	virtual ImageCaching::TryAddImageResult TryAddSourceImage(const IImageSource* image) override;

	/// <summary>
	/// Tries to remove the provided image from the cache.
	/// </summary>
	/// <param name="image">The image to remove.</param>
	/// <returns>True if the image was removed, false if the image was not found in the cache.</returns>
	virtual bool TryRemoveImage(const TImage* image) override;

private:

	void OnDestroy(const TImage* image)
	{
		TryRemoveImage(image);
		delete image;
	}

	// Function to create a shared_ptr with a callback on destruction
	std::shared_ptr<const TImage> make_shared_with_callback(const TImage* ptr) {
		ImageCache& cache = *this;
		return std::shared_ptr<const TImage>(ptr, [&cache](const TImage* image) {
			cache.OnDestroy(image);
		});
	}

public:
	virtual std::shared_ptr<const TImage> MakeSharedPtr(const TImage* image) override
	{
		return make_shared_with_callback(image);
	}
};


#include "ImageCache.inl"
