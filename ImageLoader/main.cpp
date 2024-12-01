// ImageLoader.cpp : Defines the entry point for the application.
//

#include "ImageLoader.h"
#include <iostream>
#include <filesystem>

#include "UnitTests/ImageFileLoaderTests.h"
#include "UnitTests/TestImplementations.h"

using namespace UnitTests;

int _expectedResultCount = 0;
int _expectedValidResultCount = 0;

std::atomic<int> _receivedResultCount{ 0 };

std::mutex _resultsMutex;

std::vector<std::shared_ptr<const TestImage>> _resultImages;

std::mutex _coutMutex;

void ImageLoaded(const ImageLoadTaskResult<UnitTests::TestImage> result)
{
	const auto imageSharedPtr = result.GetImage();
	const TestImage* image = imageSharedPtr.get();

	{
		std::lock_guard<std::mutex> lock(_coutMutex);
		std::cout << "image acquired:" << std::to_string(result.GetStatus()) << " ";
		if (image)
		{
			std::cout << image->GetImagePath() << " width:" << std::to_string(image->GetWidth())
				<< " height:" << std::to_string(image->GetHeight()) << " size in bytes:" << std::to_string(image->GetSizeInBytes());
		}
		std::cout << std::endl;
	}

	++_receivedResultCount;

	if (image)
	{
		std::lock_guard<std::mutex> lock(_resultsMutex);
		_resultImages.push_back(result.GetImage());
	}
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
	_expectedValidResultCount++;
	const auto tryGetStatus1 = imageLoader->TryGetImage(testImagePath1, ImageLoaded);
	assert((tryGetStatus1 == TryGetImageStatus::PlacedNewTaskInQueue, "Status was not TryGetImageStatus::PlacedNewTaskInQueue"));

	const auto testImagePath2 = testDataPath / "@base_01.jpg";
	const IImage* testImage2 = nullptr;
	_expectedResultCount++;
	_expectedValidResultCount++;
	imageLoader->TryGetImage(testImagePath2, ImageLoaded);

	const IImage* testImage1_again = nullptr;
	//_expectedResultCount++;
	const auto tryGetStatus1_again = imageLoader->TryGetImage(testImagePath1, ImageLoaded);
	//TODO: zoea 01/12/2024 this actually needs to handle that if the 1st task is done, result should be new task. Otherwise it should be already queued
	assert((tryGetStatus1 == TryGetImageStatus::TaskAlreadyExistsAndIsQueued, "Status was not TryGetImageStatus::TaskAlreadyExistsAndIsQueued"));


	const auto testImagePath3 = testDataPath / "@base (1).jpg";
	const IImage* testImage3 = nullptr;
	_expectedResultCount++;
	imageLoader->TryGetImage(testImagePath3, ImageLoaded);
	
	while (_receivedResultCount != _expectedResultCount)
	{
#ifdef _DEBUG
		const auto testCount = ImageLoader<TestImage>::_debugTaskStartedCount;
#endif
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	const auto memoryUsage = imageCache->GetCurrentMemoryUsage();
	const auto entryCount = imageCache->GetCacheEntryCount();

	assert(_resultImages.size() == _expectedValidResultCount);
	assert(entryCount == _expectedValidResultCount);

	_resultImages.clear();


	const auto memoryUsage2 = imageCache->GetCurrentMemoryUsage();
	const auto entryCount2 = imageCache->GetCacheEntryCount();

	assert(entryCount2 == 0);


	
	std::cout << "All tests completed" << std::endl;
	auto wait = std::cin.get();
	return 0;
}
