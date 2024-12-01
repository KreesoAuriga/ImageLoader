#pragma once
#include <future>
#include <cassert>
#include <filesystem>

/// <summary>
/// Container for image file data which includes the image dimensions.
/// </summary>
struct ImageData
{
	const int Width;
	const int Height;
	const unsigned char* Data;

	ImageData(int width, int height, const unsigned char* data)
		: Width(width)
		, Height(height)
		, Data(data)
	{
		assert((width >= 1, "Width must be greater than 0"));
		assert((height >= 1, "Width must be greater than 0"));
		assert((data, "data cannot be null"));
	}

	~ImageData();
};

/// <summary>
/// Interface that loads image data from a path.
/// </summary>
struct IImageDataReader
{
	/// <summary>
	/// Reads the data for an image file from a path.
	/// </summary>
	/// <param name="filePath">The absolute path to the file.</param>
	/// <returns>Image data with dimensions.</returns>
	[[nodiscard]]
	virtual ImageData* ReadFile(const std::filesystem::path& filePath) const = 0;

};


/// <summary>
/// Default implementation of the <see cref="IImageFileLoader"/> interface.
/// </summary>
class ImageDataReader : public IImageDataReader
{
public:
	/// <summary>
	/// Reads the data for an image file from a path on
	/// </summary>
	/// <param name="filePath">The absolute path to the file.</param>
	/// <returns>Image data with dimensions.</returns>
	[[nodiscard]]
	virtual ImageData* ReadFile(const std::filesystem::path& filePath) const override;
};

#include "ImageFileLoader.inl"