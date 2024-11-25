#pragma once
#include <string>
#include "Image.h"
#include "ImageCache.h"
#include <mutex>
#include <future>

//Interface for loading images from a path, optionally resized to custom dimensions.
struct IImageLoader
{
	/// <summary>
	/// Sets the maximum number of threads the loader is allowed to use for loading images. A value of 0 specifies that 
	/// the class implementation will make it's own choices about thread limitations.
	/// </summary>
	virtual void SetMaxThreadCount(int count) = 0;

	/// <summary>
	/// Attempts to get the image at the specified path. Returns false if the image could not be obtained.
	/// </summary>
	/// <param name="filePath">Path to the image.</param>
	/// <param name="outImage">If the image was obtained this will be assigned to the instance, otherwise it will be set to nullptr.</param>
	/// <returns>True if the image was successfully obtained.</returns>
	virtual std::future<const IImage*> TryGetImage(const std::filesystem::path& filePath) = 0;

	/// <summary>
	/// Attempts to get the image at the specified path, and at the specified size. Returns false if the image could not be obtained.
	/// </summary>
	/// <param name="filePath">Path to the image.</param>
	/// <param name="outImage">If the image was obtained this will be assigned to the instance, otherwise it will be set to nullptr.</param>
	/// <returns>True if the image was successfully obtained.</returns>
	virtual bool TryGetImage(const std::filesystem::path& filePath, unsigned int width, unsigned int height, const IImage*& outImage) = 0;

	/// <summary>
	/// Unloads the image, freeing up it's memory and removing it from any caching mechanisms. This function also releases any instances of 
	/// the image that have been resized.
	/// </summary>
	/// <param name="filePath">The path to the file.</param>
	virtual void ReleaseImage(const std::filesystem::path& filePath) = 0;
};

/// <summary>
/// Default implementation of the <see cref="IImageLoader"/> interface, with a cache for those images to prevent needing to
/// re-load data from the file path where possible.
/// </summary>
class ImageLoader : public IImageLoader
{

	ImageCaching::IImageCache* _imageCache;
	int _maxThreadCount;

public:
	ImageLoader(ImageCaching::IImageCache* imageCache, int maxThreadCount);

	/// <summary>
	/// Sets the maximum number of threads the loader is allowed to use for loading images. A value of 0 specifies that 
	/// the class implementation will make it's own choices about thread limitations.
	/// </summary>
	virtual void SetMaxThreadCount(int count) override;

	/// <summary>
	/// Attempts to get the image at the specified path. Returns false if the image could not be obtained.
	/// </summary>
	/// <param name="filePath">Path to the image.</param>
	/// <param name="outImage">If the image was obtained this will be assigned to the instance, otherwise it will be set to nullptr.</param>
	/// <returns>True if the image was successfully obtained.</returns>
	virtual std::future<const IImage*> TryGetImage(const std::filesystem::path& filePath) override;

	/// <summary>
	/// Attempts to get the image at the specified path, and at the specified size. Returns false if the image could not be obtained.
	/// </summary>
	/// <param name="filePath">Path to the image.</param>
	/// <param name="outImage">If the image was obtained this will be assigned to the instance, otherwise it will be set to nullptr.</param>
	/// <returns>True if the image was successfully obtained.</returns>
	virtual bool TryGetImage(const std::filesystem::path& filePath, unsigned int width, unsigned int height, const IImage*& outImage) override;

	/// <summary>
	/// Unloads the image, freeing up it's memory and removing it from any caching mechanisms. This function also releases any instances of 
	/// the image that have been resized.
	/// </summary>
	/// <param name="filePath">The path to the file.</param>
	virtual void ReleaseImage(const std::filesystem::path& filePath) override;
};