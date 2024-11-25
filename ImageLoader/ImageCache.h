#pragma once

#include "Image.h"
#include <string>
#include <map>
#include <filesystem>
#include <mutex>

namespace ImageCaching
{
	struct ImageId
	{
		const std::filesystem::path ImagePath;
		const int Width;
		const int Height;

		ImageId(const std::string* imagePath, const int width, const int height)
			: ImagePath(std::move(ImagePath))
			, Width(width)
			, Height(height)
		{
		}
	};

	/// <summary>
	/// Specifies the result of a try get operation on an implementation of an <see cref="IImageCache" /> interface.
	/// </summary>
	enum TryGetImageResult
	{
		/// <summary>
		/// The specified image was found, if a width and height were specified the found image matches those dimensions.
		/// </summary>
		FoundExactMatch,

		/// <summary>
		/// The specified image was found, but only at the original dimensions as loaded from path. A copy of that image at the
		/// specified dimensions was not found.
		/// </summary>
		FoundSourceImageOfDifferentDimensions,

		/// <summary>
		/// No image matching the path to the image was found.
		/// </summary>
		NotFound
	};

	/// <summary>
	/// Specifies the result of placing an image into the cache, which could either add new data to the cache, or update existing data.
	/// </summary>
	enum AddOrUpdateImageResult
	{
		/// <summary>
		/// The provided image was not present in the cache at any size and was added.
		/// </summary>
		Added,

		/// <summary>
		/// The provided image existed at the resolution of the source data from which it was loaded, but a copy at the resolution of the
		/// provided image was not present. The provided image was added to the cache as a resized copy of the source version.
		/// </summary>
		AddedAsResizedImage,

		/// <summary>
		/// The provided image already existed, but was a different instance and has replaced the instance in the cache.
		/// </summary>
		Updated,

		/// <summary>
		/// The provided image already existed as a resized copy of the source data from which it was loaded, but was a different instance and 
		/// has replaced the instance which is in the cache.
		/// </summary>
		UpdatedAsResizedImage,

		/// <summary>
		/// The provided image already existed in the cache, and so no change was effected.
		/// </summary>
		NoChange
	};

	struct IImageCache
	{
		/// <summary>
		/// Sets the maximum memory in bytes that the cache is allowed to use. 
		/// </summary>
		/// <param name="maximumMemoryInBytes"></param>
		virtual void SetMaxMemory(int64_t maximumMemoryInBytes) = 0;

		/// <summary>
		/// Gets the maximum memory in bytes that the cache is allowed to use.
		/// </summary>
		virtual int64_t GetMaxMemory() const = 0;

		/// <summary>
		/// Attempts to get the image identified by the specified path.
		/// </summary>
		/// <param name="imagePath">Source path of the image.</param>
		/// <param name="outImage">The image instance retrieved from the cache</param>
		/// <returns>Result of the operation</returns>
		virtual TryGetImageResult TryGetImage(const std::filesystem::path& imagePath, const IImage* outImage) = 0;

		/// <summary>
		/// Attempts to get the image identified by the specified path, with the specified width and height in pixels.
		/// </summary>
		/// <param name="imagePath">Source path of the image.</param>
		/// <param name="outImage">The image instance retrieved from the cache</param>
		/// <returns>Result of the operation</returns>
		virtual TryGetImageResult TryGetImageAtSize(const std::filesystem::path& imagePath,
			unsigned int width, unsigned int height, const IImage*& outImage) = 0;
		
		/// <summary>
		/// Adds the image to the cache, or if the image already exists in the cache but with different pixel data than the provided instance
		/// the 
		/// </summary>
		/// <param name="image">The image to add or update into the cache.</param>
		/// <returns>Result of the operation</returns>
		virtual AddOrUpdateImageResult AddOrUpdateImage(const IImage* image) = 0;

		/// <summary>
		/// Tries to remove the provided image from the cache.
		/// </summary>
		/// <param name="image">The image to remove.</param>
		/// <returns>True if the image was removed, false if the image was not found in the cache.</returns>
		virtual bool TryRemoveImage(const IImage* image) = 0;
	};

	struct ResizedImageKey
	{
		const int Width;
		const int Height;

		ResizedImageKey(const int width, const int height)
			: Width(width)
			, Height(height)
		{
		}

		std::string ToString()
		{
			std::string result = std::to_string(Width) + ":" + std::to_string(Height);
			return result;
		}
	};

	class ImageCacheItem
	{
		std::time_t _lastAccessedTime;

	public:
		const IImage* Image;

		ImageCacheItem(const IImage* image)
			: Image(image)
			, _lastAccessedTime(std::time_t())
		{
			UpdateLastAccessedTime();
		}

		/// <summary>
		/// Updates the last accessed time to the current time.
		/// </summary>
		void UpdateLastAccessedTime()
		{
			auto now = std::chrono::system_clock::now();
			_lastAccessedTime = std::chrono::system_clock::to_time_t(now);
		}

		/// <summary>
		/// Gets the last accessed time.
		/// </summary>
		/// <returns></returns>
		[[nodiscard]]
		std::time_t GetLastAccessedTime() const
		{
			return _lastAccessedTime;
		}
	};

	struct ImageCacheEntry
	{
		const std::filesystem::path ImagePath;
		ImageCacheItem SourceImageItem;

		std::map<const std::string, ImageCacheItem*>* ResizedImages = nullptr;

		ImageCacheEntry(const IImage* image)
			: ImagePath(image->GetImagePath())
			, SourceImageItem(ImageCacheItem(image))
		{
		}

		~ImageCacheEntry()
		{
			if (ResizedImages)
			{
				for (auto entry : *ResizedImages)
					delete entry.second;

				delete ResizedImages;
			}

			delete SourceImageItem.Image;
		}

		unsigned int GetTotalSizeInBytes()
		{
			unsigned int result = SourceImageItem.Image->GetSizeInBytes();

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
	class ImageCache : public IImageCache
	{
		int64_t _maxAllowedMemory;
		int64_t _currentMemoryUsage = 0;
		std::mutex _cacheLock;
		std::map<const std::string, ImageCacheEntry*> _images;

	private:
		void CheckMemoryUsage();

	public:
		ImageCache(int64_t maximumMemoryInBytes)
		{
			SetMaxMemory(maximumMemoryInBytes);
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
		/// <param name="outImage">The image instance retrieved from the cache</param>
		/// <returns>True if the image was in the cache and was retrieved, false otherwise.</returns>
		virtual TryGetImageResult TryGetImage(const std::filesystem::path& imagePath, const IImage* outImage) override;

		/// <summary>
		/// Attempts to get the image identified by the specified path, with the specified width and height in pixels.
		/// </summary>
		/// <param name="imagePath">Source path of the image.</param>
		/// <param name="outImage">The image instance retrieved from the cache</param>
		/// <returns>True if the image was in the cache and was retrieved, false otherwise.
		virtual TryGetImageResult TryGetImageAtSize(const std::filesystem::path& imagePath, 
			unsigned int width, unsigned int height, 
			const IImage*& outImage) override;

		/// <summary>
		/// Adds the image to the cache, or if the image already exists in the cache but with different pixel data than the provided instance
		/// the 
		/// </summary>
		/// <param name="image">The image to add or update into the cache.</param>
		/// <returns>Result of the operation</returns>
		virtual AddOrUpdateImageResult AddOrUpdateImage(const IImage* image) override;

		/// <summary>
		/// Tries to remove the provided image from the cache.
		/// </summary>
		/// <param name="image">The image to remove.</param>
		/// <returns>True if the image was removed, false if the image was not found in the cache.</returns>
		virtual bool TryRemoveImage(const IImage* image) override;
	};

}