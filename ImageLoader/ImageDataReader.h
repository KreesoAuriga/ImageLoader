#pragma once
#include <future>
#include <cassert>
#include <filesystem>
#include "Assert.h"

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
		ASSERT_MSG(width >= 1, "Width must be greater than 0");
		ASSERT_MSG(height >= 1, "Width must be greater than 0");
		ASSERT_MSG(data, "data cannot be null");
	}

	~ImageData();
};

/// <summary>
/// Interface that loads image data from a path.
/// </summary>
struct IImageDataReader
{
	/// <summary>
	/// Reads the data for an image file from a path. Returns null if the file is not found.
	/// </summary>
	/// <param name="filePath">The absolute path to the file.</param>
	/// <returns>Image data with dimensions.</returns>
	[[nodiscard]]
	virtual ImageData* ReadFile(const std::filesystem::path& filePath) const = 0;

	//TODO: zoea 01/12/2024 this should use a TryGet pattern, rather than just have a nullptr returned indicating file not found.
};

