#pragma once
#include "../ImageDataReader.h"
#include <future>
#include <cassert>
#include <filesystem>
#include "Assert.h"


/// <summary>
/// Default implementation of the <see cref="IImageFileLoader"/> interface.
/// </summary>
class ImageDataReader : public IImageDataReader
{
public:
	/// <summary>
	/// Reads the data for an image file from a path. Returns null if the file is not found.
	/// </summary>
	/// <param name="filePath">The absolute path to the file.</param>
	/// <returns>Image data with dimensions.</returns>
	[[nodiscard]]
	virtual ImageData* ReadFile(const std::filesystem::path& filePath) const override;
};


#include "ImageDataReader.inl"