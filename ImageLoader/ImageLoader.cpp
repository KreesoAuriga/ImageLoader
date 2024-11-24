#include "ImageLoader.h"
#include <cassert>

ImageLoader::ImageLoader(ImageCache::IImageCache* imageCache)
	: _imageCache(imageCache)

{
	assert(imageCache, "ImageCache cannot be null");
}

void ImageLoader::SetMaxThreadCount(int count)
{
}

bool ImageLoader::TryGetImage(std::string& filePath, IImage*& outImage)
{

	std::lock_guard<std::mutex> lockGuard(_cacheLock);

	if (_imageCache->TryGetImage(filePath, outImage))
		return true;



	return false;
}

bool ImageLoader::TryGetImage(std::string filePath, unsigned int width, unsigned int height, IImage*& outImage)
{
	return false;
}
