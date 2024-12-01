#pragma once
#include "../ImageFactory.h"

namespace UnitTests
{

	class TestImage final : public IImage
	{
	private:
		int _width;
		int _height;
		const std::filesystem::path _path;
		const unsigned char* _imageData;

	public:
		TestImage(int width, int height, const std::filesystem::path path, const unsigned char* imageData)
			: _width(width)
			, _height(height)
			, _path(std::move(path))
			, _imageData(imageData)
		{
		}

		~TestImage()
		{
			delete[] _imageData;
		}

		/// <summary>
		/// Gets the width in pixels of this image.
		/// </summary>
		[[nodiscard]]
		int GetWidth() const override {
			return _width;
		}

		/// <summary>
		/// Gets the height in pixels of this image.
		/// </summary>
		[[nodiscard]]
		int GetHeight() const  override {
			return _height;
		}


		/// <summary>
		/// Gets the path from which image was originally loaded. This also serves as a unique identifier in <see cref="IImageCache"/>.
		/// </summary>
		[[nodiscard]]
		std::filesystem::path GetImagePath() const override {
			return _path;
		}

		/// <summary>
		/// Gets the size in bytes of the image.
		/// </summary>
		[[nodiscard]]
		unsigned int GetSizeInBytes() const override {
			return _width * _height * 4;
		}
	};

	class ImageFactory : public IImageFactory<TestImage>
	{
		/// <summary>
		/// Constructs a TestImage from raw 8 bit rgba image data. 
		/// </summary>
		/// <param name="width">Width in pixels of the image.</param>
		/// <param name="height">Height in pixels of the image.</param>
		/// <param name="rgbaData">Pointer to the raw image data.</param>
		/// <returns>New instance of the image type.</returns>
		[[nodiscard]]
		virtual const TestImage* ConstructImage(const int width, const int height, const std::filesystem::path& path, const unsigned char* rgbaData)
		{
			return new TestImage(width, height, path, rgbaData);
		}
	};
}