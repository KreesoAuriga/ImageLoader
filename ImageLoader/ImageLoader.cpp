#include "Image.h"
#include "ImageLoader.h"
#include "ImageFileLoader.h"
#include <cassert>
#include <future>
#include <thread>
#include <iostream>


ImageLoader::ImageLoader(ImageCaching::IImageCache* imageCache)
	: _imageCache(imageCache)

{
	assert(imageCache, "ImageCache cannot be null");
}

void ImageLoader::SetMaxThreadCount(int count)
{
}

bool ImageLoader::TryGetImage(const std::filesystem::path& filePath, const IImage*& outImage)
{

	std::lock_guard<std::mutex> lockGuard(_cacheLock);

	if (_imageCache->TryGetImage(filePath, outImage))
		return true;


	std::promise<ImageData*> promise;
	std::future<ImageData*> future = promise.get_future();

    std::thread t([&promise, filePath]
    {
        try
        {
            auto imageFileLoader = new ImageDataReader();
            const auto fileData = imageFileLoader->LoadFile(filePath);

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
    }
    catch (const std::exception& e)
    {
        std::cout << "Exception from the thread: " << e.what() << '\n';
        return false;
    }

    t.join();

    outImage = new Image(filePath, imageFileData->Width, imageFileData->Height, imageFileData->Data);
    _imageCache->

	return true;
}

bool ImageLoader::TryGetImage(const std::filesystem::path& filePath, int width, int height, const IImage*& outImage)
{
	return false;
}
