#include "ImageLoader.h"
#include "Image.h"
#include "ImageFileLoader.h"
#include <cassert>
#include <future>
#include <thread>
#include <iostream>


ImageLoader::ImageLoader(ImageCaching::IImageCache* imageCache, int maxThreadCount)
	: _imageCache(imageCache)
    , _maxThreadCount(maxThreadCount)

{
	assert((imageCache, "ImageCache cannot be null"));
}

void ImageLoader::SetMaxThreadCount(int count)
{
    _maxThreadCount = count;
}

struct ThreadContainer
{
    std::promise<const ImageData*> promise;

};


std::future<const IImage*> ImageLoader::TryGetImage(const std::filesystem::path& filePath)
{    
    std::promise<const IImage*> imagePromise;
    std::future<const IImage*> imageFuture = imagePromise.get_future();
    
    const IImage* cachedImage = nullptr;
    if (_imageCache->TryGetImage(filePath, cachedImage) != ImageCaching::TryGetImageResult::NotFound)
    {
        imagePromise.set_value(cachedImage);
        return imageFuture;
    }

    auto* imageCache = _imageCache;
    std::thread t([imagePromise = std::move(imagePromise), filePath, imageCache]() mutable
    {
      
        try
        {
            auto imageFileLoader = new ImageDataReader();
            const auto fileData = imageFileLoader->ReadFile(filePath);

            const auto* image = new Image(filePath, fileData->Width, fileData->Height, fileData->Data);
            const auto tryAddResult = imageCache->TryAddImage(image);
            if (tryAddResult == ImageCaching::TryAddImageResult::NoChange)
            {
                //image is already in the cache, probably from another thread doing the same work.
                imageCache->TryGetImage(filePath, image);
            }

            imagePromise.set_value(image);
        }
        catch (...)
        {
            try
            {
                // store anything thrown in the promise
                imagePromise.set_exception(std::current_exception());
            }
            catch (...) {} // set_exception() may throw too
        }

    });

    t.detach();

    return imageFuture;
}

bool ImageLoader::TryGetImage(const std::filesystem::path& filePath, unsigned int width, unsigned int height, const IImage*& outImage)
{
    return false;
}


void ImageLoader::ReleaseImage(const std::filesystem::path& filePath)
{
}
