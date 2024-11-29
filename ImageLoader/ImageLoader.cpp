#include "ImageLoader.h"
#include "Image.h"
#include "ImageFileLoader.h"
#include <cassert>
#include <future>
#include <thread>
#include <iostream>
#include <type_traits>



template<typename TImage>
void ImageLoader<TImage>::Update(ImageLoader<TImage>* imageLoader)
{
    while (!imageLoader->_abort)
    {
        std::lock_guard<std::recursive_mutex> taskQueueLock(imageLoader->_taskQueueMutex);

        if (imageLoader->_currentThreadCount < imageLoader->_maxThreadCount)
        {
            if (!imageLoader->_taskQueue.empty())
            {
                for (const auto& queueItem : imageLoader->_taskQueue)
                {
                    //just start one task per pass
                    LoadImageTask* task = queueItem.second;
                    if (!task->IsStarted)
                    {
                        task->IsStarted = true;
                        _currentThreadCount++;
                        std::thread thread(&LoadImageTask::StartLoad, task);
                        thread.detach();
                        break;
                    }
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

template<typename TImage>
void ImageLoader<TImage>::SignalThreadCompleted(LoadImageTask* loadImageTask)
{
    std::lock_guard<std::recursive_mutex> taskQueueLock(ImageLoader->_taskQueueMutex);
    _taskQueue.erase(loadImageTask->Identifier);
    _currentThreadCount--;
    delete loadImageTask;
}

template<typename TImage>
void ImageLoader<TImage>::SetMaxThreadCount(int count)
{
    _maxThreadCount = count;
}

template<typename TImage>
const void ImageLoader<TImage>::TryGetImage(
    const std::filesystem::path& filePath,
    std::function<void(ImageLoadTaskResult<TImage>)> imageLoadedCallback)
{    
    TryGetImage(filePath, 0, 0, imageLoadedCallback);
}

template<typename TImage>
const void ImageLoader<TImage>::TryGetImage(
    const std::filesystem::path& filePath,
    unsigned int width,
    unsigned int height,
    std::function<void(ImageLoadTaskResult<TImage>)> imageLoadedCallback)
{
    std::lock_guard<std::recursive_mutex> taskQueueLock(_taskQueueMutex);

    //don't make a new task for the requested image and size if one is already queued.
    const auto key = filePath.string();
    if (auto task = _taskQueue.find(key); task != _taskQueue.end())
    {
        return;
    }

    auto task = new LoadImageTask(key, filePath, width, height, this, _imageCache, imageLoadedCallback);
    _taskQueue[key] = task;
}

template<typename TImage>

void ImageLoader<TImage>::ReleaseImage(const std::filesystem::path& filePath)
{
    throw std::runtime_error("Not implemented");
}


template<typename TImage>
void ImageLoader<TImage>::LoadImageTask::StartLoad()
{
    bool success = false;
    try
    {
        std::lock_guard<std::mutex> lockGuard(Mutex);

        //std::shared_ptr<const TImage> loadedImage;
        //const IImageSource* imageSource = nullptr;

        ImageCaching::TryGetImageResult tryGetResult;
        if (Width <= 0 && Height <= 0)
            tryGetResult = _imageCache->TryGetImage(FilePath, LoadedImage, ImageSource);
        else
            tryGetResult = _imageCache->TryGetImageAtSize(FilePath, Width, Height, LoadedImage, ImageSource);

        switch (tryGetResult)
        {
        case ImageCaching::TryGetImageResult::FoundExactMatch:
        {
            const auto result = ImageLoadTaskResult(ImageLoadStatus::Success, LoadedImage, "");
            _returnedCallback(result);
            return;
        }

        case ImageCaching::TryGetImageResult::FoundSourceImageOfDifferentDimensions:
            {
                Resize();
                return;
            }

        case ImageCaching::TryGetImageResult::NotFound:
            {
                auto imageFileLoader = new ImageDataReader();
                const ImageData* fileData = nullptr;
                try
                {
                    fileData = imageFileLoader->ReadFile(FilePath);
                }
                catch (std::exception ex)
                {
                    const auto result = ImageLoadTaskResult(ImageLoadStatus::FailedToLoad, LoadedImage, ex.what());
                    _returnedCallback(result);
                    _imageLoader->SignalThreadCompleted(this);
                    return;
                }

                const ImageSource* image = new ImageSource(FilePath, fileData->Width, fileData->Height, fileData->Data);
                fileData->Data = nullptr;
                delete fileData;
                fileData = nullptr;

                const auto tryAddResult = ImageCache->TryAddImage(image);
                if (tryAddResult == ImageCaching::TryAddImageResult::NoChange)
                {
                    //image is already in the cache, probably from another thread doing the same work.
                    delete image;//wasn't added to the cache, this is a duplicate.
                }

                Resize();
                _imageLoader->SignalThreadCompleted(this);
                return;
            }

        default:
            throw std::runtime_error("Unknown value for TryGetResult");
        }

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

    _imageLoader->SignalThreadCompleted(this);
}

template<typename TImage>
void ImageLoader<TImage>::LoadImageTask::Resize()
{
    if( !SourceImage )//sanity check
        throw std::runtime_error("Resize image failed because SourceImage has not been set.");

    const TImage* image = _imageFactory->ConstructImage(Width, Height, FilePath, SourceImage->GetPixels());
    if (!image)
    {
        const auto result = ImageLoadTaskResult(ImageLoadStatus::FailedToLoad, LoadedImage, "Image factory returned nullptr.");
        _returnedCallback(result);
        return;
    }

    const TImage* existingImage = nullptr;
    const auto tryAddResult = ImageCache->TryAddImage(image, existingImage);
    if (tryAddResult == ImageCaching::TryAddImageResult::NoChange)
    {
        LoadedImage = existingImage;
        if (existingImage != image)
        {
            //image is already in the cache, probably from another thread doing the same work.
            delete image;//wasn't added to the cache, this is a duplicate.
        }
    }


    const auto result = ImageLoadTaskResult(ImageLoadStatus::Success, LoadedImage, "");
    _returnedCallback(result);
}


/*
getSize(source)
   return TImage* from factory

tryget from cache
  get lock by path as key
  open lock
     redo tryget incase another thread did the work

     if cache doesn't have source item
       create loader task that resizes
         places resized in cache and gets the sharedPtr
       return the task

     if cache has source but not size
       create task that resizes
         places resized in cache and get the sharedPtr
       return the task

*/