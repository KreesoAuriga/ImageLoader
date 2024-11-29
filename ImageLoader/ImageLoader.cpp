#include "ImageLoader.h"
#include "Image.h"
#include "ImageFileLoader.h"
#include <cassert>
#include <future>
#include <thread>
#include <iostream>
#include <type_traits>



template<typename TImage>
void ImageLoader<TImage>::SetMaxThreadCount(int count)
{
    _maxThreadCount = count;
}

template<typename TImage>
std::shared_ptr<const TImage*> ImageLoader<TImage>::TryGetImage(const std::filesystem::path& filePath)
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

            const auto* image = new ImageSource(filePath, fileData->Width, fileData->Height, fileData->Data);
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

template<typename TImage>
bool ImageLoader<TImage>::TryGetImage(const std::filesystem::path& filePath, unsigned int width, unsigned int height, const IImage*& outImage)
{
    throw std::runtime_error("Not implemented");
}

template<typename TImage>

void ImageLoader<TImage>::ReleaseImage(const std::filesystem::path& filePath)
{
    throw std::runtime_error("Not implemented");
}

template<typename TImage>
void ImageLoader<TImage>::LoadImageTask::Start()
{
    try
    {
        std::lock_guard<std::mutex> lockGuard(Mutex);

        auto imageFileLoader = new ImageDataReader();
        const auto fileData = imageFileLoader->ReadFile(FilePath);

        const ImageSource* image = new ImageSource(FilePath, fileData->Width, fileData->Height, fileData->Data);
        const auto tryAddResult = ImageCache->TryAddImage(image);
        if (tryAddResult == ImageCaching::TryAddImageResult::NoChange)
        {
            //image is already in the cache, probably from another thread doing the same work.
            delete image;//wasn't added to the cache, this is a duplicate.
        }

        LoadedImage = image;
    }
    catch (...)
    {
        try
        {
            // store anything thrown in the promise
            //imagePromise.set_exception(std::current_exception());
        }
        catch (...) {} // set_exception() may throw too
    }
}

template<typename TImage>
const TImage* ImageLoader<TImage>::LoadImageTask::GetImageIfCompleted()
{
    return LoadedImage;
}
