#pragma once

#include "Image.h"
#include <string>
#include <map>

namespace ImageCache
{

	struct IImageCache
	{
		virtual void SetMaxMemory(unsigned int maximumMemoryInBytes) = 0;
		virtual void GetMaxMemory() = 0;

		/// <summary>
		/// Attempts to get the image from the specified path
		/// </summary>
		/// <param name="imagePath"></param>
		/// <param name="outImage"></param>
		/// <returns></returns>
		virtual bool TryGetImage(std::string imagePath, const IImage* outImage) = 0;

		virtual bool TryGetImageAtSize(std::string imagePath, unsigned int width, unsigned int height, const IImage*& outImage) = 0;

		virtual bool TryAddImage(std::string imagePath, const IImage* outImage) = 0;
	};


	struct ImageId
	{
		const std::string ImagePath;
		const unsigned int Width;
		const unsigned int Height;

		ImageId(const std::string* imagePath, const unsigned int width, const unsigned int height)
			: ImagePath(std::move(ImagePath))
			, Width(width)
			, Height(height)
		{
		}
	};


	class ImageCache : public IImageCache
	{
		std::map<std::string, IImage*> _images;



	};

}