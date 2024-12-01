#include "ImageLoader.h"
#include "Image.h"
#include "ImageDataReader.h"
#include "ImageSource.h"
#include <cassert>
#include <future>
#include <thread>
#include <iostream>
#include <type_traits>


template<typename TImage>
void ImageLoader<TImage>::Update(ImageLoader<TImage>* imageLoader)
{
    while (!imageLoader->_updateThreadAbort)
    {
        std::lock_guard<std::recursive_mutex> taskQueueLock(imageLoader->_taskQueueMutex);

        //This should be thread safe, because we only increment running thread count within the taskqueue mutex.
        //The thread for a running task could decrement the value after we read it, but the priority is we don't launch
        //too many threads, not that we launch the exact number of non-running threads.
        int availableThreadCount = imageLoader->_maxThreadCount - imageLoader->_runningThreadsCount;
        if (availableThreadCount > 0 && !imageLoader->_taskQueue.empty())
         {
            for (int t = 0; t < availableThreadCount; t++)
            {
                for (const auto& queueItem : imageLoader->_taskQueue)
                {
                    LoadImageTask* task = queueItem.second;
                    if (!task->IsStarted)
                    {
                        task->IsStarted = true;

                        std::thread thread(&LoadImageTask::StartAndDelete, task);
                        imageLoader->SignalThreadStart(task);
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
void ImageLoader<TImage>::SignalThreadStart(LoadImageTask* loadImageTask)
{
    const auto count = ++_runningThreadsCount;
    if (count > _maxThreadCount)
    {
        const auto msg = "Started a new thread when the max thread count of " + std::to_string(_maxThreadCount) + " had already been reached.";
        throw std::runtime_error(msg);
    }
}

template<typename TImage>
void ImageLoader<TImage>::SignalThreadCompleted(LoadImageTask* loadImageTask)
{
    std::lock_guard<std::recursive_mutex> taskQueueLock(_taskQueueMutex);
    const int threadCount = _runningThreadsCount;
    std::cout << "Task completed, current threadCount=" << threadCount << "\n";
    _taskQueue.erase(loadImageTask->Identifier);

    _runningThreadsCount--;
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
    std::string errorMessage = "";
    try
    {
        std::lock_guard<std::mutex> lockGuard(Mutex);

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

                fileData = imageFileLoader->ReadFile(FilePath);
                

                if (!fileData)
                    throw std::runtime_error("The specified file was not found.");

                SourceImage = new ImageSource(FilePath, fileData->Width, fileData->Height, fileData->Data);
                Width = fileData->Width;
                Height = fileData->Height;
                fileData->Data = nullptr;
                delete fileData;
                fileData = nullptr;

                const auto tryAddResult = ImageCache->TryAddSourceImage(SourceImage);
                switch (tryAddResult)
                {
                    case ImageCaching::TryAddImageResult::Added:
                        result = Resize();
                        success = true;
                        break;

                    case ImageCaching::TryAddImageResult::AddedAsResizedImage:
                        //note: shoud never happen because addsource never returns this value.
                        break;

                    case ImageCaching::TryAddImageResult::NoChange:
                        //image is already in the cache, probably from another thread doing the same work.
                        delete SourceImage;//wasn't added to the cache, this is a duplicate.
                        break;

                    case ImageCaching::TryAddImageResult::OutOfMemory:
                        errorMessage += "ImageCache is out of memory. ";
                        break;

                    default:
                        throw std::runtime_error("Unknown value for ImageCaching::TryAddImageResult");
                }
                break;
            }

        default:
            throw std::runtime_error("Unknown value for TryGetResult");
        }

    }
    catch (std::exception ex)
    {
        //TODO: zoea 01/12/2024 - actually store the exception on the ImageLoadTaskResult.
         errorMessage += ex.what();
    }

    if (!success)
    {
        errorMessage = FilePath.string() + " " + errorMessage;
        result = ImageLoadTaskResult<TImage>(ImageLoadStatus::FailedToLoad, nullptr, errorMessage);
    }

    _imageLoader->SignalThreadCompleted(this);
    _returnedCallback(result);
    delete this;
}

template<typename TImage>
ImageLoadTaskResult<TImage> ImageLoader<TImage>::LoadImageTask::Resize()
{
    if( !SourceImage )//sanity check
        throw std::runtime_error("Resize image failed because SourceImage has not been set.");


    //For sake of testing, resize will just be implemented as truncating the byte stream to the matching size.
    //Visually this result is incorrect, but this test code has no affordance to display the images anyway.
    int resizedLength = Width * Height * 4;//rgba 8bits per color
    auto* pixelDataAtSize = new unsigned char[resizedLength];

    const auto* sourceData = SourceImage->GetPixels();
    memcpy(pixelDataAtSize, sourceData, resizedLength);

    const TImage* image = this->_imageLoader->_imageFactory->ConstructImage(Width, Height, FilePath, pixelDataAtSize);
    if (!image)
    {
        return ImageLoadTaskResult(ImageLoadStatus::FailedToLoad, LoadedImage, "Image factory returned nullptr.");
    }

    const TImage* existingImage = nullptr;
    LoadedImage = ImageCache->MakeSharedPtr(image);
    const auto tryAddResult = ImageCache->TryAddImage(LoadedImage, existingImage);
    if (tryAddResult == ImageCaching::TryAddImageResult::NoChange)
    {
        //sanity check - this shouldn't actually happen. Investigate if it does.
        throw std::runtime_error("Image at size already existed");
    }


    return ImageLoadTaskResult(ImageLoadStatus::Success, LoadedImage, "");
}
