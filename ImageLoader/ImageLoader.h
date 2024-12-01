#pragma once
#include <string>
#include "Image.h"
#include "ImageCache.h"
#include "ImageFactory.h"
#include <mutex>
#include <future>
#include <functional>
#include <cassert>

enum ImageLoadStatus
{
	Success,
	FailedToLoad,
	OutOfMemory
};

namespace std
{
	static std::string to_string(ImageLoadStatus imageLoadStatus)
	{
		switch (imageLoadStatus)
		{
		case ImageLoadStatus::Success:
			return "Success";

		case ImageLoadStatus::FailedToLoad:
			return "FailedToLoad";

		case ImageLoadStatus::OutOfMemory:
			return "OutOfMemory";

		default:
			return "OUT OF RANGE VALUE for ImageLoadStatus";
		}
	}
}

enum TryGetImageStatus
{
	PlacedNewTaskInQueue,
	TaskAlreadyExistsAndIsQueued
};

template<typename TImage>
class ImageLoadTaskResult 
{
	ImageLoadStatus _status;
	std::shared_ptr<const TImage> _imageResult;
	std::string _errorMessage;

public:
	ImageLoadTaskResult()
		: _status(ImageLoadStatus::FailedToLoad)
		, _imageResult(std::shared_ptr<const TImage>(nullptr))
		, _errorMessage("")
	{
	}

	ImageLoadTaskResult(ImageLoadStatus status, std::shared_ptr<const TImage> imageResult, const std::string errorMessage)
		: _status(status)
		, _imageResult(std::move(imageResult))
		, _errorMessage(std::move(errorMessage))
	{
	}

	[[nodiscard]]
	ImageLoadStatus GetStatus() const {
		return _status;
	}

	[[nodiscard]]
	std::shared_ptr<const TImage> GetImage() const {
		return _imageResult;
	}

	[[nodiscard]]
	std::string GetErrorMessage() const
	{
		return _errorMessage;
	}
};

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
	virtual const TryGetImageStatus TryGetImage(
		const std::filesystem::path& filePath, 
		std::function<void(const ImageLoadTaskResult<TImage>)> imageLoadedCallback) = 0;

	/// <summary>
	/// Attempts to get the image at the specified path, and at the specified size. Returns false if the image could not be obtained.
	/// </summary>
	/// <param name="filePath">Path to the image.</param>
	/// <param name="outImage">If the image was obtained this will be assigned to the instance, otherwise it will be set to nullptr.</param>
	/// <returns>True if the image was successfully obtained.</returns>
	virtual const TryGetImageStatus TryGetImage(const std::filesystem::path& filePath, unsigned int width, unsigned int height,
		std::function<void(const ImageLoadTaskResult<TImage>)> imageLoadedCallback) = 0;

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
#ifdef _DEBUG
public:
	//remove before final checkin
	static int _debugTaskStartedCount;
#endif

public:
	//[[nodiscard]]
	//using ImageReturnedCallback = std::function<void(std::shared_ptr<const TImage>())>;
	//using ImageReturnedCallback = void(std::shared_ptr<const TImage>);

private:


	ImageCaching::IImageCache<TImage>* _imageCache;
	IImageFactory<TImage>* _imageFactory;
	int _maxThreadCount = 1;
	int _currentThreadCount = 0;

	std::thread* _updateThread = nullptr;
	bool _updateThreadAbort = false;

	class LoadImageTask
	{

	public:
		std::string Identifier;
		std::mutex Mutex;
		std::condition_variable Condition;
		const std::filesystem::path FilePath;
		int Width;
		int Height;
		const IImageSource* SourceImage = nullptr;
		std::shared_ptr<const TImage> LoadedImage;
		ImageCaching::IImageCache<TImage>* ImageCache;
		ImageLoader<TImage>* _imageLoader;
		std::function<void(const ImageLoadTaskResult<TImage>)> _returnedCallback;


		bool IsStarted = false;

		LoadImageTask(std::string identifier,
			std::filesystem::path filePath, int width, int height,
			ImageLoader<TImage>* imageLoader,
			ImageCaching::IImageCache<TImage>* imageCache,
			std::function<void(const ImageLoadTaskResult<TImage>)> returnedCallback)
			: Identifier(identifier)
			, FilePath(std::move(filePath))
			, Width(width)
			, Height(height)
			, _imageLoader(imageLoader)
			, ImageCache(imageCache)
			, _returnedCallback(std::move(returnedCallback))

		{
		}

		void StartAndDelete();
	private:
		[[nodiscard]]
		ImageLoadTaskResult<TImage> Resize();

		//std::shared_ptr<const TImage>* GetImageIfCompleted();
	};

	std::recursive_mutex _taskQueueMutex;
	std::map<std::string, LoadImageTask*> _taskQueue;

	std::recursive_mutex _imageLocksMutex;
	std::map<std::string, std::recursive_mutex*> _imageLocks;

	std::recursive_mutex* GetImageLock(const std::filesystem::path& filePath)
	{
		std::lock_guard(_imageLocksMutex) lock;

		const auto key = filePath.string();
		if (auto search = _imageLocks.find(key); search != _imageLocks.end())
		{
			return search.second;
		}

		auto* result = new std::recursive_mutex();
		_imageLocks[key] = result;
		return result;
	}

private:
	static void Update(ImageLoader<TImage>* imageLoader);
	void SignalThreadCompleted(LoadImageTask* loadImageTask);

public:
	ImageLoader(ImageCaching::IImageCache<TImage>* imageCache, IImageFactory<TImage>* imageFactory, int maxThreadCount)
		: _imageCache(imageCache)
		, _imageFactory(imageFactory)
		, _maxThreadCount(maxThreadCount)

	{
		assert((imageCache, "ImageCache cannot be null"));
		static_assert(std::is_convertible<TImage*, IImage*>::value, "TImage type must inherit from IImage.");

		_updateThread = new std::thread(Update, this);
		_updateThread->detach();
	}

	~ImageLoader()
	{
		_updateThreadAbort = true;
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
	virtual const TryGetImageStatus TryGetImage(
		const std::filesystem::path& filePath, 		
		std::function<void(ImageLoadTaskResult<TImage>)> imageLoadedCallback) override;

	/// <summary>
	/// Attempts to get the image at the specified path, and at the specified size. Returns false if the image could not be obtained.
	/// </summary>
	/// <param name="filePath">Path to the image.</param>
	/// <param name="outImage">If the image was obtained this will be assigned to the instance, otherwise it will be set to nullptr.</param>
	/// <returns>True if the image was successfully obtained.</returns>
	virtual const TryGetImageStatus TryGetImage(
		const std::filesystem::path& filePath, 
		unsigned int width,
		unsigned int height,
		std::function<void(ImageLoadTaskResult<TImage>)> imageLoadedCallback) override;

	/// <summary>
	/// Unloads the image, freeing up it's memory and removing it from any caching mechanisms. This function also releases any instances of 
	/// the image that have been resized.
	/// </summary>
	/// <param name="filePath">The path to the file.</param>
	virtual void ReleaseImage(const std::filesystem::path& filePath) override;

};

#include "ImageLoader.inl"