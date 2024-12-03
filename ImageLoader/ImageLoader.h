#pragma once
#include <cassert>
#include <functional>
#include <future>
#include <mutex>
#include <string>
#include "Assert.h"
#include "Image.h"
#include "ImageCache.h"
#include "ImageFactory.h"

enum ImageLoadStatus
{
	Success,
	FailedToLoad,
	OutOfMemory
};

namespace std
{
	static std::string to_string(const ImageLoadStatus imageLoadStatus)
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
	{
	}

	ImageLoadTaskResult(const ImageLoadStatus status, std::shared_ptr<const TImage> imageResult, const std::string errorMessage)
		: _status(status)
		, _imageResult(std::move(imageResult))
		, _errorMessage(std::move(errorMessage))
	{
	}

	/// <summary>
	/// Gets the status of the completed operation.
	/// </summary>
	ImageLoadStatus GetStatus() const {
		return _status;
	}

	/// <summary>
	/// Gets the image loaded by the task.
	/// </summary>
	[[nodiscard]]
	std::shared_ptr<const TImage> GetImage() const {
		return _imageResult;
	}

	/// <summary>
	/// Gets the error message, or an empty string if not applicable.
	/// </summary>
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
protected:
	~IImageLoader() = default;

public:
	/// <summary>
	/// Sets the maximum number of threads the loader is allowed to use for loading images. A value of 0 specifies that 
	/// the class implementation will make it's own choices about thread limitations.
	/// </summary>
	virtual void SetMaxThreadCount(int count) = 0;

	/// <summary>
	/// Attempts to get the image at the specified path. Returns false if the image could not be obtained.
	/// </summary>
	/// <param name="filePath">Path to the image.</param>
	/// <param name="imageLoadedCallback">Callback that will be invoked completion, returning an ImageLoadTaskResult.</param>
	/// <returns>Status of the operation.</returns>
	virtual TryGetImageStatus TryGetImage(
		const std::filesystem::path& filePath, 
		std::function<void(const ImageLoadTaskResult<TImage>)> imageLoadedCallback) = 0;

	/// <summary>
	/// Attempts to get the image at the specified path, and at the specified size. Returns false if the image could not be obtained.
	/// </summary>
	/// <param name="filePath">Path to the image.</param>
	/// <param name="width">The width in pixels of the image to be retrieved.</returns>
	/// <param name="height">The height in pixels of the image to be retrieved.</returns>
	/// <param name="imageLoadedCallback">Callback that will be invoked completion, returning an ImageLoadTaskResult.</param>
	/// <returns>Status of the operation.</returns>
	virtual TryGetImageStatus TryGetImage(const std::filesystem::path& filePath, unsigned int width, unsigned int height,
		std::function<void(const ImageLoadTaskResult<TImage>)> imageLoadedCallback) = 0;

	/// <summary>
	/// Unloads the image, freeing up it's memory and removing it from any caching mechanisms. This function also releases any instances of 
	/// the image that have been resized.
	/// </summary>
	/// <param name="filePath">The path to the file.</param>
	virtual void ReleaseImage(const std::filesystem::path& filePath) = 0;
};
