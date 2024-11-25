#pragma once
#include <filesystem>

enum ImageFormat
{	
	RGB8,
	RGBA8
};

struct IImage
{
public:
	/// <summary>
	/// Gets the width in pixels of this image.
	/// </summary>
	[[nodiscard]]
	virtual int GetWidth() const = 0;

	/// <summary>
	/// Gets the height in pixels of this image.
	/// </summary>
	[[nodiscard]]
	virtual int GetHeight() const = 0;

	/// <summary>
	/// Gets the <see cref="ImageFormat"/> of this image as stored in memory.
	/// </summary>
	[[nodiscard]]
	virtual ImageFormat GetNativeImageFormat() const = 0;

	/// <summary>
	/// Gets the image data pixels, as raw image data
	/// </summary>
	/// <param name="imageFormat">The format to return the image in.</param>
	[[nodiscard]]
	virtual const unsigned char* GetPixels(ImageFormat imageFormat) const = 0;

	/// <summary>
	/// Gets the path from which image was originally loaded.
	/// </summary>
	[[nodiscard]]
	virtual std::filesystem::path GetImagePath() const = 0;

	/// <summary>
	/// Gets the size in bytes of the image.
	/// </summary>
	[[nodiscard]]
	virtual unsigned int GetSizeInBytes() const = 0;
};

struct IImageResized : public IImage
{
	/// <summary>
	/// Gets the parent image that this instance was resized from
	/// </summary>
	[[nodiscard]]
	virtual const IImage* GetParentImage() = 0;
};

class Image : public IImage
{
protected:
	int _width;
	int _height;
	const unsigned char* _imageData;

	const std::filesystem::path _sourcePath;

public:
	Image(const std::filesystem::path& sourcePath, int width, int height, const unsigned char* imageData)
		: _sourcePath(sourcePath)
		, _width(width)
		, _height(height)
		, _imageData(imageData)
	{
	}

	~Image()
	{
		delete[] _imageData;
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
	/// Gets the <see cref="ImageFormat"/> of this image as stored in memory.
	/// </summary>
	[[nodiscard]]
	virtual ImageFormat GetNativeImageFormat() const override {
		return ImageFormat::RGB8;
	}

	/// <summary>
	/// Gets the image data pixels, as raw image data
	/// </summary>
	/// <param name="imageFormat">The format to return the image in.</param>
	[[nodiscard]]
	virtual const unsigned char* GetPixels(ImageFormat imageFormat) const override {
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

class ImageResized : public Image
{
private:
	const Image* _parentImage;

public:
	ImageResized(const Image* parentImage, unsigned int width, unsigned int height, const unsigned char* imageData)
		: _parentImage(parentImage)
		, Image(parentImage->GetImagePath(), width, height, imageData)
	{
	}
};