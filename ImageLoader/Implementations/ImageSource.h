#pragma once
#include "../Image.h"

class ImageSource : public IImageSource
{
protected:
	int _width;
	int _height;
	const unsigned char* _imageData;

	const std::filesystem::path _sourcePath;

public:
	ImageSource(const std::filesystem::path& sourcePath, const int width, const int height, const unsigned char* imageData)
		: _sourcePath(sourcePath)
		, _width(width)
		, _height(height)
		, _imageData(imageData)
	{
	}

	~ImageSource() override
	{
		free((void*)_imageData);
	}

	/// <summary>
	/// Gets the width in pixels of this image.
	/// </summary>
	[[nodiscard]]
	virtual int GetWidth() const override {
		return _width;
	}

	/// <summary>
	/// Gets the height in pixels of this image.
	/// </summary>
	[[nodiscard]]
	virtual int GetHeight() const override {
		return _height;
	}

	/// <summary>
	/// Gets the image data pixels, as raw image data
	/// </summary>
	[[nodiscard]]
	virtual const unsigned char* GetPixels() const override {
		return _imageData;
	}

	/// <summary>
	/// Gets the path from which image was originally loaded.
	/// </summary>
	[[nodiscard]]
	virtual std::filesystem::path GetImagePath() const override {
		return _sourcePath;
	}

	/// <summary>
	/// Gets the size in bytes of the image.
	/// </summary>
	[[nodiscard]]
	virtual unsigned int GetSizeInBytes() const override
	{
		//TODO: zoea 25/11/2024 make this properly handle images with channel count other than 4 and 
		//pixels more than 8 bits per channel
		return _width * _height * 4;
	}
};


inline IImageSource::~IImageSource() = default;
