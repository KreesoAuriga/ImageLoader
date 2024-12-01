#pragma once
#include <string>
#include "Image.h"
#include "ImageCache.h"
#include "ImageFactory.h"
#include <mutex>
#include <future>
#include <functional>
#include <cassert>
#include "Assert.h"

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
