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
	enum TryAddImageResult
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
		/// The provided image already existed in the cache at the provided key, and so no change was effected.
		/// </summary>
		NoChange
	};

	template<typename TImage>
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
		virtual TryGetImageResult TryGetImage(const std::filesystem::path& imagePath, const TImage* outImage) = 0;

		/// <summary>
		/// Attempts to get the image identified by the specified path, with the specified width and height in pixels.
		/// </summary>
		/// <param name="imagePath">Source path of the image.</param>
		/// <param name="outImage">The image instance retrieved from the cache</param>
		/// <returns>Result of the operation</returns>
		virtual TryGetImageResult TryGetImageAtSize(const std::filesystem::path& imagePath,
			unsigned int width, unsigned int height, const TImage*& outImage) = 0;
		
		/// <summary>
		/// Adds the image to the cache, unless the image already exists in the cache at the image's path.
		/// </summary>
		/// <param name="image">The image to add into the cache.</param>
		/// <returns>Result of the operation</returns>
		virtual TryAddImageResult TryAddImage(const TImage* image) = 0;

		/// <summary>
		/// Tries to remove the provided image from the cache.
		/// </summary>
		/// <param name="image">The image to remove.</param>
		/// <returns>True if the image was removed, false if the image was not found in the cache.</returns>
		virtual bool TryRemoveImage(const TImage* image) = 0;
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

	template<typename TImage>
	class ImageCacheItem
	{

		std::weak_ptr<const TImage> _image;

	public:

		ImageCacheItem(const std::shared_ptr<const IImage>& image)
			: _image(image)
		{
		}

		std::shared_ptr<const IImage> GetImage()
		{
			return _image.lock();
		}

	};

	template<typename TImage>
	struct ImageCacheEntry
	{

		const std::filesystem::path ImagePath;
		const ImageSource* SourceImage;
		//ImageCacheItem SourceImageItem;

		std::map<const std::string, ImageCacheItem<TImage>*> ResizedImages;

		ImageCacheEntry(const ImageSource* sourceImage)
			: ImagePath(sourceImage->GetImagePath())
			, SourceImage(ImageCacheItem<TImage>(sourceImage))
		{
			static_assert(std::is_convertible<TImage*, IImage*>::value, "TImage type must inherit from IImage.");
		}

		~ImageCacheEntry()
		{
			if (ResizedImages)
			{
				for (auto entry : *ResizedImages)
					delete entry.second;

				delete ResizedImages;
			}

			delete SourceImage;
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
	class ImageCache : public IImageCache<TImage>
	{
		int64_t _maxAllowedMemory;
		int64_t _currentMemoryUsage = 0;
		std::recursive_mutex _cacheLock;
		std::map<const std::string, ImageCacheEntry<TImage>*> _images;

	private:
		void CheckMemoryUsage();



	public:
		ImageCache(int64_t maximumMemoryInBytes)
		{
			static_assert(std::is_convertible< TImage*, IImage*>::value, "TImage type must inherit from IImage.");
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
		virtual TryGetImageResult TryGetImage(const std::filesystem::path& imagePath, const TImage* outImage) override;

		/// <summary>
		/// Attempts to get the image identified by the specified path, with the specified width and height in pixels.
		/// </summary>
		/// <param name="imagePath">Source path of the image.</param>
		/// <param name="outImage">The image instance retrieved from the cache</param>
		/// <returns>True if the image was in the cache and was retrieved, false otherwise.
		virtual TryGetImageResult TryGetImageAtSize(const std::filesystem::path& imagePath, 
			unsigned int width, unsigned int height, 
			const TImage*& outImage) override;

		/// <summary>
		/// Adds the image to the cache, unless the image already exists in the cache at the image's path.
		/// </summary>
		/// <param name="image">The image to add into the cache.</param>
		/// <returns>Result of the operation</returns>
		virtual TryAddImageResult TryAddImage(const TImage* image) override;

		virtual TryAddImageResult TryAddSourceImage(const IImageSource* image) override;

		/// <summary>
		/// Tries to remove the provided image from the cache.
		/// </summary>
		/// <param name="image">The image to remove.</param>
		/// <returns>True if the image was removed, false if the image was not found in the cache.</returns>
		virtual bool TryRemoveImage(const TImage* image) override;

	private:

		// Function to create a shared_ptr with a callback on destruction
		std::shared_ptr<const TImage> make_shared_with_callback(const TImage* ptr){//, std::function<void()> onDestroy) {
			return std::shared_ptr<const TImage>(ptr, [onDestroy](const TImage* obj) {
				TryRemoveImage(obj);
				delete obj;          // Clean up the object
				//if (onDestroy) {     // Call the callback
				//	onDestroy();
				//}
			});
		}

	};

}