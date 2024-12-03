#pragma once
#include <cassert>
#include <functional>
#include <future>
#include <mutex>
#include <string>
#include <utility>

#include "../Assert.h"
#include "../Image.h"
#include "ImageCache.h"
#include "../ImageFactory.h"
#include "../ImageLoader.h"


/// <summary>
/// Default implementation of the <see cref="IImageLoader"/> interface, with a cache for those images to prevent needing to
/// re-load data from the file path where possible.
/// </summary>
template<typename TImage>
class ImageLoader final : public IImageLoader<TImage>
{

private:


	ImageCaching::IImageCache<TImage>* _imageCache;
	IImageFactory<TImage>* _imageFactory;
	int _maxThreadCount = 1;

	std::thread* _updateThread = nullptr;
	bool _updateThreadAbort = false;


	std::atomic<int> _runningThreadsCount = 0;

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
		ImageLoader<TImage>* Loader;
		std::function<void(const ImageLoadTaskResult<TImage>)> ReturnedCallback;


		bool IsStarted = false;

		LoadImageTask(std::string identifier,
			std::filesystem::path filePath, const int width, const int height,
			ImageLoader<TImage>* imageLoader,
			ImageCaching::IImageCache<TImage>* imageCache,
			std::function<void(const ImageLoadTaskResult<TImage>)> returnedCallback)
			: Identifier(std::move(identifier))
			, FilePath(std::move(filePath))
			, Width(width)
			, Height(height)
			, ImageCache(imageCache)
			, Loader(imageLoader)
			, ReturnedCallback(std::move(returnedCallback))
		{
		}

		void StartAndDelete();
	private:
		[[nodiscard]]
		ImageLoadTaskResult<TImage> Resize();
	};

	std::recursive_mutex _taskQueueMutex;
	std::map<std::string, LoadImageTask*> _taskQueue;

	std::recursive_mutex _imageLocksMutex;
	std::map<std::string, std::recursive_mutex*> _imageLocks;

	std::recursive_mutex* GetImageLock(const std::filesystem::path& filePath)
	{
		std::lock_guard lock(_imageLocksMutex);

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
	void SignalThreadStart(LoadImageTask* loadImageTask);
	void SignalThreadCompleted(LoadImageTask* loadImageTask);



public:
	ImageLoader(ImageCaching::IImageCache<TImage>* imageCache, IImageFactory<TImage>* imageFactory, const int maxThreadCount)
		: _imageCache(imageCache)
		, _imageFactory(imageFactory)
		, _maxThreadCount(maxThreadCount)

	{
		ASSERT_MSG(imageCache, "ImageCache cannot be null");
		static_assert(std::is_convertible_v<TImage*, IImage*>, "TImage type must inherit from IImage.");

		_updateThread = new std::thread(Update, this);
		_updateThread->detach();
	}

	~ImageLoader()
	{
		_updateThreadAbort = true;

		//the update thread gets lock on the task queue each pass, so we'll use the same mutex to wait for it to exit.
		std::lock_guard<std::recursive_mutex> taskQueueLock(_taskQueueMutex);
	}

	int GetRunningThreadsCount() const {
		return _runningThreadsCount;
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
	/// <param name="imageLoadedCallback">Callback that will be invoked completion, returning an ImageLoadTaskResult.</param>
	/// <returns>Status of the operation.</returns>
	virtual TryGetImageStatus TryGetImage(
		const std::filesystem::path& filePath,
		std::function<void(ImageLoadTaskResult<TImage>)> imageLoadedCallback) override;

	/// <summary>
	/// Attempts to get the image at the specified path, and at the specified size. Returns false if the image could not be obtained.
	/// </summary>
	/// <param name="filePath">Path to the image.</param>
	/// <param name="width">The width in pixels of the image to be retrieved.</returns>
	/// <param name="height">The height in pixels of the image to be retrieved.</returns>
	/// <param name="imageLoadedCallback">Callback that will be invoked completion, returning an ImageLoadTaskResult.</param>
	/// <returns>Status of the operation.</returns>
	virtual TryGetImageStatus TryGetImage(
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