#pragma once
#include <future>
#include <cassert>

/// <summary>
/// Container for file data which includes the size.
/// </summary>
struct ImageFileData
{
	const int Width;
	const int Height;
	const unsigned char* Data;

	ImageFileData(int width, int height, const unsigned char* data)
		: Width(width)
		, Height(height)
		, Data(data)
	{
		assert(width >= 1, "Width must be greater than 0");
		assert(height >= 1, "Width must be greater than 0");
		assert(data, "data cannot be null");
	}

	~ImageFileData()
	{
		delete[] Data;
	}
};

struct IImageFileLoader
{
	virtual const ImageFileData* LoadFile(const std::string& filePath) const = 0;
};

class ImageFileLoader : public IImageFileLoader
{
public:
	virtual const ImageFileData* LoadFile(const std::string& filePath) const override;
};