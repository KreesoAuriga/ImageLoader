#pragma once
#include <filesystem>
#include <iostream>


struct IImage
{
protected:
	~IImage() = default;

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
	/// Gets the path from which image was originally loaded. This also serves as a unique identifier in <see cref="IImageCache"/>.
	/// </summary>
	[[nodiscard]]
	virtual std::filesystem::path GetImagePath() const = 0;

	/// <summary>
	/// Gets the size in bytes of the image.
	/// </summary>
	[[nodiscard]]
	virtual unsigned int GetSizeInBytes() const = 0;
};

struct IImageSource : public IImage
{
public:
	virtual ~IImageSource() = 0;

	/// <summary>
	/// Gets the image data pixels, as raw image data
	/// </summary>
	[[nodiscard]]
	virtual const unsigned char* GetPixels() const = 0;

};


