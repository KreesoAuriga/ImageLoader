
#pragma once

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
	virtual unsigned int GetWidth() const = 0;

	/// <summary>
	/// Gets the height in pixels of this image.
	/// </summary>
	[[nodiscard]]
	virtual unsigned int GetHeight() const = 0;

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
	virtual char* GetPixels(ImageFormat imageFormat) const = 0;

	/// <summary>
	/// Gets the path from which image was originally loaded.
	/// </summary>
	[[nodiscard]]
	virtual std::string GetImagePath() const = 0;
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
	unsigned int _width;
	unsigned int _height;
	char* _imageData;

	std::string _sourcePath;

public:
	Image(const std::string& sourcePath, unsigned int width, unsigned int height, char* imageData)
		: _sourcePath(std::move(sourcePath))
		, _width(width)
		, _height(height)
		, _imageData(imageData)
	{
	}
};

class ImageResized : public Image
{
private:
	const Image* _parentImage;

public:
	ImageResized(const Image* parentImage, unsigned int width, unsigned int height, char* imageData)
		: _parentImage(parentImage)
		, Image(parentImage->GetImagePath(), width, height, imageData)
	{
	}
};