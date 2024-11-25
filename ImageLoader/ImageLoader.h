#pragma once
#include <string>
#include "Image.h"
#include "ImageCache.h"
#include <mutex>

struct IImageLoader
{
	/// <summary>
	/// Sets the maximum number of threads the loader is allowed to use for loading images. A value of 0 specifies that 
	/// the class implementation will handle thread limitations.
	/// </summary>
	virtual void SetMaxThreadCount(int count) = 0;

	/// <summary>
	/// Attempts to get the image at the specified path. Returns false if the image could not be obtained.
	/// </summary>
	/// <param name="filePath">Path to the image.</param>
	/// <param name="outImage">If the image was obtained this will be assigned to the instance, otherwise it will be set to nullptr.</param>
	/// <returns>True if the image was successfully obtained.</returns>
	virtual bool TryGetImage(const std::string& filePath, IImage*& outImage) = 0;

	/// <summary>
	/// Attempts to get the image at the specified path, and at the specified size. Returns false if the image could not be obtained.
	/// </summary>
	/// <param name="filePath">Path to the image.</param>
	/// <param name="outImage">If the image was obtained this will be assigned to the instance, otherwise it will be set to nullptr.</param>
	/// <returns>True if the image was successfully obtained.</returns>
	virtual bool TryGetImage(const std::string& filePath, unsigned int width, unsigned int height, IImage*& outImage) = 0;

	/// <summary>
	/// Unloads the image, freeing up it's memory and removing it from any caching mechanisms.
	/// </summary>
	/// <param name="filePath">The file path of the file.</param>
	virtual void ReleaseImage(const std::string& filePath) = 0;
};

class ImageLoader : public IImageLoader
{
	ImageCache::IImageCache* _imageCache;
	std::mutex _cacheLock;

private:

public:
	ImageLoader(ImageCache::IImageCache* imageCache);

	virtual void SetMaxThreadCount(int count) override;

	virtual bool TryGetImage(const std::string& filePath, const IImage*& outImage) override;

	virtual bool TryGetImage(const std::string& filePath, unsigned int width, unsigned int height, const IImage*& outImage) override;

};