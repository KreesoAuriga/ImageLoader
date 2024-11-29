#pragma once
#include <string>
#include "Image.h"
#include "ImageCache.h"
#include "ImageFactory.h"
#include <mutex>
#include <future>

//Interface for loading images from a path, optionally resized to custom dimensions.
template<typename TImage>
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
template<typename TImage>
class ImageLoader : public IImageLoader<TImage>
{

	ImageCaching::IImageCache<TImage>* _imageCache;
	IImageFactory<TImage>* _imageFactory;
	int _maxThreadCount;

	class LoadImageTask
	{
		std::mutex Mutex;
		std::condition_variable Condition;
		const std::filesystem::path FilePath;
		int Width;
		int Height;
		const TImage* LoadedImage = nullptr;
		ImageCaching::IImageCache<TImage>* ImageCache;

	public:

		LoadImageTask(std::filesystem::path filePath, int width, int height, ImageCaching::IImageCache<TImage>* imageCache)
			: FilePath(std::move(filePath))
			, Width(width)
			, Height(height)
			, ImageCache(imageCache)
		{
		}

		void Start();

		const TImage* GetImageIfCompleted();
	};

	std::map<std::string, LoadImageTask> _taskQueue;

public:
	ImageLoader::ImageLoader(ImageCaching::IImageCache<TImage>* imageCache, IImageFactory<TImage>* imageFactory, int maxThreadCount)
		: _imageCache(imageCache)
		, _imageFactory(imageFactory)
		, _maxThreadCount(maxThreadCount)

	{
		assert((imageCache, "ImageCache cannot be null"));
		static_assert(std::is_convertible<TImage*, IImage*>::value, "TImage type must inherit from IImage.");
	}

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
	virtual std::shared_ptr<const TImage*> TryGetImage(const std::filesystem::path& filePath) override;

	/// <summary>
	/// Attempts to get the image at the specified path, and at the specified size. Returns false if the image could not be obtained.
	/// </summary>
	/// <param name="filePath">Path to the image.</param>
	/// <param name="outImage">If the image was obtained this will be assigned to the instance, otherwise it will be set to nullptr.</param>
	/// <returns>True if the image was successfully obtained.</returns>
	virtual bool TryGetImage(const std::filesystem::path& filePath, unsigned int width, unsigned int height, const TImage*& outImage) override;

	/// <summary>
	/// Unloads the image, freeing up it's memory and removing it from any caching mechanisms. This function also releases any instances of 
	/// the image that have been resized.
	/// </summary>
	/// <param name="filePath">The path to the file.</param>
	virtual void ReleaseImage(const std::filesystem::path& filePath) override;

};