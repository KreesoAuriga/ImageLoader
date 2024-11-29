// ImageLoader.cpp : Defines the entry point for the application.
//

#include "ImageLoader.h"
#include <iostream>
#include <filesystem>

#include "UnitTests/ImageFileLoaderTests.h"
#include "UnitTests/TestImplementations.h"

using namespace UnitTests;

static int _expectedResultCount = 0;

std::atomic<int> _receivedResultCount{ 0 };

std::mutex _resultsMutex;

std::vector<std::shared_ptr<const TestImage>> _resultImages;

void ImageLoaded(const ImageLoadTaskResult<UnitTests::TestImage> result)
{
	const auto imageSharedPtr = result.GetImage();
	const TestImage* image = imageSharedPtr.get();

	std::cout << "image acquired:" << image->GetImagePath() << " width:" << std::to_string(image->GetWidth())
		<< " height:" << std::to_string(image->GetHeight()) << " size in bytes:" << std::to_string(image->GetSizeInBytes()) << std::endl;

	++_receivedResultCount;

	std::lock_guard<std::mutex> lock(_resultsMutex);
	_resultImages.push_back(result.GetImage());
}

int main()
{
	const auto testDataPath = std::filesystem::path("F:\\dev\\Serato Interview\\ImageLoader\\TestData");


	std::cout << "Initialize unit tests" << std::endl;
	UnitTests::UnitTestsSetup::Initialize(testDataPath);
	auto imageFileLoaderTests = new UnitTests::ImageFileLoaderTests();

	const auto testResultMessages = imageFileLoaderTests->LoadFiles();
	for (const auto& message : testResultMessages)
		std::cout << message << std::endl;


	auto imageFactory = new UnitTests::ImageFactory();
	auto imageCache = new ImageCaching::ImageCache<TestImage>(64 * 1024 * 1024);
	auto imageLoader = new ImageLoader<TestImage>(imageCache, imageFactory, 4);


	const auto testImagePath1 = testDataPath / "@Response_05.bmp";
	const IImage* testImage1 = nullptr;
	_expectedResultCount++;
	imageLoader->TryGetImage(testImagePath1, ImageLoaded);

	const auto testImagePath2 = testDataPath / "@base (1).jpg";
	const IImage* testImage2 = nullptr;
	_expectedResultCount++;
	imageLoader->TryGetImage(testImagePath2, ImageLoaded);

	const IImage* testImage1_again = nullptr;
	_expectedResultCount++;
	imageLoader->TryGetImage(testImagePath1, ImageLoaded);

	while (_receivedResultCount != _expectedResultCount)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	const auto memoryUsage = imageCache->GetCurrentMemoryUsage();
	const auto entryCount = imageCache->GetCacheEntryCount();

	assert(_resultImages.size() == _expectedResultCount);
	assert(entryCount == 2);

	_resultImages.clear();


	const auto memoryUsage2 = imageCache->GetCurrentMemoryUsage();
	const auto entryCount2 = imageCache->GetCacheEntryCount();

	assert(entryCount == 0);


	
	std::cout << "All tests completed" << std::endl;
	auto wait = std::cin.get();
	return 0;
}
