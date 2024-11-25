#include "ImageLoader.h"
#include "Image.h"
#include "ImageFileLoader.h"
#include <cassert>
#include <future>
#include <thread>
#include <iostream>


ImageLoader::ImageLoader(ImageCaching::IImageCache* imageCache)
	: _imageCache(imageCache)

{
	assert((imageCache, "ImageCache cannot be null"));
}

void ImageLoader::SetMaxThreadCount(int count)
{
}

bool ImageLoader::TryGetImage(const std::filesystem::path& filePath, const IImage*& outImage)
{

	//std::lock_guard<std::mutex> lockGuard(_cacheLock);

	if (_imageCache->TryGetImage(filePath, outImage) != ImageCaching::TryGetImageResult::NotFound )
		return true;


	std::promise<const ImageData*> promise;
	std::future<const ImageData*> future = promise.get_future();

    std::thread t([&promise, filePath]
    {
        try
        {
            auto imageFileLoader = new ImageDataReader();
            const auto fileData = imageFileLoader->ReadFile(filePath);
            promise.set_value(fileData);

            // code that may throw
            //throw std::runtime_error("Example");
        }
        catch (...)
        {
            try
            {
                // store anything thrown in the promise
                promise.set_exception(std::current_exception());
            }
            catch (...) {} // set_exception() may throw too
        }
    });

    const ImageData* imageFileData = nullptr;
    try
    {
        imageFileData = future.get();
        imageFileData = imageFileData;
    }
    catch (const std::exception& e)
    {
        std::cout << "Exception from the thread: " << e.what() << '\n';
        return false;
    }

    t.join();

    outImage = new Image(filePath, imageFileData->Width, imageFileData->Height, imageFileData->Data);
    _imageCache->AddOrUpdateImage(outImage);

	return true;
}

bool ImageLoader::TryGetImage(const std::filesystem::path& filePath, unsigned int width, unsigned int height, const IImage*& outImage)
{
    return false;
}


void ImageLoader::ReleaseImage(const std::filesystem::path& filePath)
{
}
