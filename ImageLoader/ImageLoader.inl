#include "ImageLoader.h"
#include "Image.h"
#include "ImageFileLoader.h"
#include <cassert>
#include <future>
#include <thread>
#include <iostream>
#include <type_traits>


#ifdef _DEBUG
//remove before final checkin
template<typename TImage>
int ImageLoader<TImage>::_debugTaskStartedCount = 0;
#endif

template<typename TImage>
void ImageLoader<TImage>::Update(ImageLoader<TImage>* imageLoader)
{
    while (!imageLoader->_updateThreadAbort)
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
                        imageLoader->_currentThreadCount++;
                        std::thread thread(&LoadImageTask::StartAndDelete, task);
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
    std::lock_guard<std::recursive_mutex> taskQueueLock(_taskQueueMutex);
    _taskQueue.erase(loadImageTask->Identifier);
    _currentThreadCount--;
}

template<typename TImage>
void ImageLoader<TImage>::SetMaxThreadCount(int count)
{
    _maxThreadCount = count;
}

template<typename TImage>
const TryGetImageStatus ImageLoader<TImage>::TryGetImage(
    const std::filesystem::path& filePath,
    std::function<void(ImageLoadTaskResult<TImage>)> imageLoadedCallback)
{    
    return TryGetImage(filePath, 0, 0, imageLoadedCallback);
}

template<typename TImage>
const TryGetImageStatus ImageLoader<TImage>::TryGetImage(
    const std::filesystem::path& filePath,
    unsigned int width,
    unsigned int height,
    std::function<void(ImageLoadTaskResult<TImage>)> imageLoadedCallback)
{
    std::lock_guard<std::recursive_mutex> taskQueueLock(_taskQueueMutex);

    //don't make a new task for the requested image and size if one is already queued.
    const auto sizeKey = ResizedImageKey(width, height);
    const auto key = filePath.string() + ":" + sizeKey.ToStringKey();

    if (auto task = _taskQueue.find(key); task != _taskQueue.end())
    {
        return TryGetImageStatus::TaskAlreadyExistsAndIsQueued;
    }

    auto task = new LoadImageTask(key, filePath, width, height, this, _imageCache, imageLoadedCallback);
    _taskQueue[key] = task;

    return TryGetImageStatus::PlacedNewTaskInQueue;
}

template<typename TImage>

void ImageLoader<TImage>::ReleaseImage(const std::filesystem::path& filePath)
{
    throw std::runtime_error("Not implemented");
}


template<typename TImage>
void ImageLoader<TImage>::LoadImageTask::StartAndDelete()
{
    bool success = false;
    ImageLoadTaskResult<TImage> result = ImageLoadTaskResult<TImage>();
    try
    {
        std::lock_guard<std::mutex> lockGuard(Mutex);

        ImageLoader<TImage>::_debugTaskStartedCount++;
        //std::shared_ptr<const TImage> loadedImage;
        //const IImageSource* imageSource = nullptr;

        ImageCaching::TryGetImageResult tryGetResult;
        if (Width <= 0 && Height <= 0)
            tryGetResult = ImageCache->TryGetImage(FilePath, LoadedImage, SourceImage);
        else
            tryGetResult = ImageCache->TryGetImageAtSize(FilePath, Width, Height, LoadedImage, SourceImage);

        switch (tryGetResult)
        {
        case ImageCaching::TryGetImageResult::FoundExactMatch:
        {
            result = ImageLoadTaskResult<TImage>(ImageLoadStatus::Success, LoadedImage, "");
            break;
        }

        case ImageCaching::TryGetImageResult::FoundSourceImageOfDifferentDimensions:
            {
                result = Resize();
                break;
            }

        case ImageCaching::TryGetImageResult::NotFound:
            {
                auto imageFileLoader = new ImageDataReader();
                ImageData* fileData = nullptr;
                try
                {
                    fileData = imageFileLoader->ReadFile(FilePath);
                }
                catch (std::exception ex)
                {
                    const auto result = ImageLoadTaskResult<TImage>(ImageLoadStatus::FailedToLoad, LoadedImage, ex.what());
                    _returnedCallback(result);
                    _imageLoader->SignalThreadCompleted(this);
                    return;
                }

                SourceImage = new ImageSource(FilePath, fileData->Width, fileData->Height, fileData->Data);
                Width = fileData->Width;
                Height = fileData->Height;
                fileData->Data = nullptr;
                delete fileData;
                fileData = nullptr;

                const auto tryAddResult = ImageCache->TryAddSourceImage(SourceImage);
                if (tryAddResult == ImageCaching::TryAddImageResult::NoChange)
                {
                    //image is already in the cache, probably from another thread doing the same work.
                    delete SourceImage;//wasn't added to the cache, this is a duplicate.
                }

                result = Resize();
                break;
            }

        default:
            throw std::runtime_error("Unknown value for TryGetResult");
        }

        success = true;

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

    if (!success)
        result = ImageLoadTaskResult<TImage>(ImageLoadStatus::FailedToLoad, nullptr, "Unexpected error");

    _imageLoader->SignalThreadCompleted(this);
    _returnedCallback(result);
    delete this;
}

template<typename TImage>
ImageLoadTaskResult<TImage> ImageLoader<TImage>::LoadImageTask::Resize()
{
    if( !SourceImage )//sanity check
        throw std::runtime_error("Resize image failed because SourceImage has not been set.");

    const TImage* image = this->_imageLoader->_imageFactory->ConstructImage(Width, Height, FilePath, SourceImage->GetPixels());
    if (!image)
    {
        return ImageLoadTaskResult(ImageLoadStatus::FailedToLoad, LoadedImage, "Image factory returned nullptr.");
    }

    const TImage* existingImage = nullptr;
    LoadedImage = ImageCache->MakeSharedPtr(image);
    const auto tryAddResult = ImageCache->TryAddImage(LoadedImage, existingImage);
    if (tryAddResult == ImageCaching::TryAddImageResult::NoChange)
    {
        throw std::runtime_error("fix this");
        /*LoadedImage = existingImage;
        if (existingImage != image)
        {
            //image is already in the cache, probably from another thread doing the same work.
            delete image;//wasn't added to the cache, this is a duplicate.
        }*/
    }


    return ImageLoadTaskResult(ImageLoadStatus::Success, LoadedImage, "");
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