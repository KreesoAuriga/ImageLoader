#pragma once

#include "Image.h"
#include <string>
#include <map>
#include <filesystem>
#include <mutex>

namespace ImageCaching
{
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
		NoChange,

		OutOfMemory
	};

	template<typename TImage>
	struct IImageCache
	{
	protected:
		~IImageCache() = default;

	public:
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
		/// <param name="outImage">The image instance if type TImage retrieved from the cache or loaded from the path.</param>
		/// <param name="outSourceImage">The source image instance retrieved from the cache or loaded from the path.</param>
		/// <returns>Result of the operation</returns>
		virtual TryGetImageResult TryGetImage(const std::filesystem::path& imagePath,
			std::shared_ptr<const TImage>& outImage,
			const IImageSource*& outSourceImage) = 0;

		/// <summary>
		/// Attempts to get the image identified by the specified path, with the specified width and height in pixels.
		/// </summary>
		/// <param name="imagePath">Source path of the image.</param>
		/// <param name="width">The width in pixels of the image to be retrieved.</returns>
		/// <param name="height">The height in pixels of the image to be retrieved.</returns>
		/// <param name="outImage">The image instance if type TImage retrieved from the cache or loaded from the path.</param>
		/// <param name="outSourceImage">The source image instance retrieved from the cache or loaded from the path.</param>
		/// <returns>Result of the operation</returns>
		virtual TryGetImageResult TryGetImageAtSize(const std::filesystem::path& imagePath,
			unsigned int width, unsigned int height,
			std::shared_ptr<const TImage>& outImage,
			const IImageSource*& outSourceImage) = 0;

		/// <summary>
		/// Constructs a shared pointer to the image instance, adding a custom deleter if required.
		/// </summary>
		/// <param name="image">The image.</param>
		virtual std::shared_ptr<const TImage> MakeSharedPtr(const TImage* image) = 0;

		/// <summary>
		/// Adds the image to the cache, unless the image already exists in the cache at the image's path.
		/// If there is already an image in the cache for the image's path, outImage will be the instance that that was in the cache.
		/// </summary>
		/// <param name="image">The image to add into the cache.</param>
		/// <param name="outImage">If a different instance already exists in the cache at the image's path, this will be set to that instance.
		/// Otherwise is nullptr.</param>
		/// <returns>Result of the operation</returns>
		virtual TryAddImageResult TryAddImage(std::shared_ptr<const TImage> image, const TImage*& outImage) = 0;

		/// <summary>
		/// Adds the source image containing the pixel data of the source file to the cache, unless the source image already 
		/// exists in the cache.
		/// </summary>
		/// <param name="image">The source image</param>
		/// <returns>Result of the operation</returns>
		virtual TryAddImageResult TryAddSourceImage(const IImageSource* image) = 0;

		/// <summary>
		/// Tries to remove the provided image from the cache.
		/// </summary>
		/// <param name="image">The image to remove.</param>
		/// <returns>True if the image was removed, false if the image was not found in the cache.</returns>
		virtual bool TryRemoveImage(const TImage* image) = 0;
	};

}