// ImageLoader.cpp : Defines the entry point for the application.
//

#include "ImageLoader.h"
#include <iostream>
#include <filesystem>

#include "UnitTests/ImageFileLoaderTests.h"
#include "UnitTests/TestImplementations.h"
#include "Assert.h"

using namespace UnitTests;

std::filesystem::path _testDataPath;

int _expectedImageLoadResultCount = 0;
int _expectedImageLoadWasValidResultCount = 0;

std::atomic<int> _imageLoadResultCount{ 0 };
std::atomic<int> _imageLoadWasValidCount{ 0 };
std::atomic<int> _imageLoadNotEnoughMemory{ 0 };

std::mutex _resultsMutex;

/// <summary>
/// Will store the loaded images, both to confirm loading, but also to retain a reference to each.
/// Images in the cache are delete when the shared_ptr deletes them, so the code using ImageLoader to load the images
/// is responsible for retaining the result returned from the loading tasks.
/// </summary>
std::vector<std::shared_ptr<const TestImage>> _resultImages;

std::mutex _coutMutex;

ImageCache<TestImage>* _imageCache = nullptr;
ImageLoader<TestImage>* _imageLoader = nullptr;

void OnImageLoaded(const ImageLoadTaskResult<UnitTests::TestImage> result)
{
	const auto imageSharedPtr = result.GetImage();
	const TestImage* image = imageSharedPtr.get();

	{
		//lock against cout to ensure the entire message is output at once, without getting mixed in with other
		//threads hitting this function concurrently.
		std::lock_guard<std::mutex> lock(_coutMutex);
		const auto status = result.GetStatus();

		//note that the memory usage and active threads count are for debugging info usage, and are not guaranteed to be correct because
		//other threads could modify the values before this reports, and because the callback is invoked after the thread ends.
		std::string imageCacheMemoryUsageMessage = " ImageCache memory usage:" + std::to_string(_imageCache->GetCurrentMemoryUsage());
		std::string activeThreadsMessage = " active threads:" + std::to_string(_imageLoader->GetRunningThreadsCount());
		switch (status)
		{
		case ImageLoadStatus::Success:
		{
			std::cout << std::to_string(status) << " - image acquired:";
			std::cout << image->GetImagePath() << " width:" << std::to_string(image->GetWidth());
			std::cout << " height:" << std::to_string(image->GetHeight()) << " size in bytes:" << std::to_string(image->GetSizeInBytes());
			std::cout << "\n";
			std::cout << imageCacheMemoryUsageMessage << "\n";
			std::cout << "\n";
		}
			break;
		case ImageLoadStatus::FailedToLoad:
		{
			std::cout << std::to_string(status) << " - image failed to load:";
			std::cout << result.GetErrorMessage();
			std::cout << "\n";
			std::cout << imageCacheMemoryUsageMessage << "\n";
			std::cout << "\n";
		}
			break;
		case ImageLoadStatus::OutOfMemory:
		{
			++_imageLoadNotEnoughMemory;
			std::cout << std::to_string(status) << " - image was not loaded because it would exceed the specified limit of memory for loaded images. ";
			std::cout << "\n";
			std::cout << result.GetErrorMessage();
			std::cout << "\n";
			std::cout << imageCacheMemoryUsageMessage << "\n";
			std::cout << "\n";
		}
			break;
		default:
			break;
		}
	}

	++_imageLoadResultCount;

	if (image)
	{
		++_imageLoadWasValidCount;
		std::lock_guard<std::mutex> lock(_resultsMutex);
		_resultImages.push_back(result.GetImage());
	}
}



void AcceptanceTest(int maxThreadCount, bool testNotEnoughMemory)
{

	_resultImages.clear();
	_expectedImageLoadResultCount = 0;
	_expectedImageLoadWasValidResultCount = 0;

	_imageLoadResultCount = 0;
	_imageLoadWasValidCount = 0;
	_imageLoadNotEnoughMemory = 0;

	int totalMemoryOfSourceFiles = 0;

	const std::string base01_filename = "@base_01.jpg";
	const std::string base01_baseFilename = "@base_01 ";
	int base01_memorySize = 1920 * 1080 * 4;

	std::vector<std::string> test_filenames;
	for (int i = 1; i < 33; i++)
	{
		test_filenames.emplace_back(base01_baseFilename + "(" + std::to_string(i) + ").jpg");
		totalMemoryOfSourceFiles += base01_memorySize;
	}

	const std::string response01_filename = "@Response_05.bmp";
	int response01_memorySize = 1920 * 1080 * 4;
	totalMemoryOfSourceFiles += response01_memorySize;
	test_filenames.push_back(response01_filename);

	const std::string mechtaTwo_filename = "@09c_Mechta_two_Version_c4_Red_I2f_2_alt.bmp";
	int mechtaTwo_memorySize = 1920 * 1080 * 4;
	totalMemoryOfSourceFiles += mechtaTwo_memorySize;
	test_filenames.push_back(mechtaTwo_filename);

	const std::string nightclubV_filename = "@Nightclub_V_starburst_bloomzoom.bmp";
	int nightclubV_memorySize = 1920 * 1080 * 4;
	totalMemoryOfSourceFiles += nightclubV_memorySize;
	test_filenames.push_back(nightclubV_filename);

	const std::string nonexistent_filename = "@does_not_exist.jpg";
	test_filenames.push_back(nonexistent_filename);

	//max memory is the image sizes in pixels, x4 channels of 8bit, plus some padding, and then doubled because the
	//cache stores the source image, and the actual IImage instance created using this data by the image factory.
	int maxMemory = (totalMemoryOfSourceFiles + 1024) * 2;
	if (testNotEnoughMemory)
	{
		maxMemory /= 2;
	}

	_expectedImageLoadWasValidResultCount = 35;//32 base01 jpg, 3 bmp, one nonexistent filename,
	_expectedImageLoadResultCount = 36;//plus one nonexistent file 

	auto imageFactory = new UnitTests::ImageFactory();
	_imageCache = new ImageCaching::ImageCache<TestImage>(maxMemory);
	_imageLoader = new ImageLoader<TestImage>(_imageCache, imageFactory, maxThreadCount);

	for (const auto& fileName : test_filenames)
	{
		const auto testFilePath = _testDataPath / fileName;
		const auto tryGetStatus = _imageLoader->TryGetImage(testFilePath, OnImageLoaded);
		ASSERT_MSG(tryGetStatus == TryGetImageStatus::PlacedNewTaskInQueue, "Status was not TryGetImageStatus::PlacedNewTaskInQueue");
	}

	while (_imageLoadResultCount != _expectedImageLoadResultCount)
	{
		const auto runningThreadsCount = _imageLoader->GetRunningThreadsCount();
		ASSERT_MSG(runningThreadsCount <= maxThreadCount, "running threads count exceeds the max threadcount of:" + std::to_string(maxThreadCount));
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	if (testNotEnoughMemory)
	{
		std::cout << "assert: out of memory result occurred more than once: " << _imageLoadNotEnoughMemory << " > 0 \n";
		ASSERT(_imageLoadNotEnoughMemory > 0);
	}

	const auto cacheMemoryUsage = _imageCache->GetCurrentMemoryUsage();
	const auto cacheEntryCount = _imageCache->GetCacheEntryCount();

	std::cout << "assert: cacheMemoryUsage < maxMemory: " << cacheMemoryUsage << " < " << maxMemory << "\n";
	ASSERT(cacheMemoryUsage < maxMemory);

	std::cout << "assert: loaded images == expected: " << _resultImages.size() << " == " << _expectedImageLoadWasValidResultCount << "\n";
	ASSERT(_resultImages.size() == _expectedImageLoadWasValidResultCount);

	std::cout << "assert: number of cache entries == expected: " << cacheEntryCount << " == " << _expectedImageLoadWasValidResultCount << "\n";
	ASSERT(cacheEntryCount == _expectedImageLoadWasValidResultCount);

	std::cout << "assert: image load attempts == expected: " << _imageLoadResultCount << " == " << _expectedImageLoadResultCount << "\n";
	ASSERT(_imageLoadResultCount == _expectedImageLoadResultCount);

	//clear the list of shared_ptrs to loaded results. This should result in the removal of these items from the cache
	_resultImages.clear();

	std::cout << "assert: cache memory is clear after images have been released in main test code.\n";
	const auto cacheMemoryUsageAfterClear = _imageCache->GetCurrentMemoryUsage();

	std::cout << "assert: cache has no entries after images have been released in main test code.\n";
	const auto cacheEntryCountAfterClear = _imageCache->GetCacheEntryCount();


	delete imageFactory;
	delete _imageCache;
	delete _imageLoader;
}


int main()
{
	_testDataPath = std::filesystem::current_path() / ".." / ".." / ".." / ".." / "TestData";// path("F:\\dev\\Serato Interview\\ImageLoader\\TestData");

	{
		std::cout << "Initialize unit tests" << "\n";
		UnitTests::UnitTestsSetup::Initialize(_testDataPath);
		{
			//file load unit tests
			auto imageFileLoaderTests = new UnitTests::ImageFileLoaderTests();

			const auto testResultMessages = imageFileLoaderTests->LoadFiles();
			for (const auto& message : testResultMessages)
				std::cout << message << "\n";
		}

		std::cout << "Finished unit tests : press enter to continue" << "\n";
		auto wait = std::cin.get();
	}


	{
		auto start = std::chrono::high_resolution_clock::now();
		std::cout << "Acceptance test #1, 4 allowed loader threads, allow enough memory for all images. \n";
		AcceptanceTest(4, false);

		auto end = std::chrono::high_resolution_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start);

		std::cout << "Acceptance test #1 elapsed time:" << elapsed << " : press enter to continue" << "\n";
		auto wait = std::cin.get();
	}

	{
		auto start = std::chrono::high_resolution_clock::now();
		std::cout << "Acceptance test #2, 16 allowed loader threads, allow enough memory for all images. \n";
		AcceptanceTest(16, false);

		auto end = std::chrono::high_resolution_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start);

		std::cout << "Acceptance test #2 elapsed time:" << elapsed << " : press enter to continue" << "\n";
		auto wait = std::cin.get();
	}

	{
		auto start = std::chrono::high_resolution_clock::now();
		std::cout << "Acceptance test #3, 4 allowed loader threads, cache size does not have capacity for all images. \n";
		AcceptanceTest(4, false);

		auto end = std::chrono::high_resolution_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start);

		std::cout << "Acceptance test #3 elapsed time:" << elapsed << " : press enter to continue" << "\n";
		auto wait = std::cin.get();
	}

	{
		auto start = std::chrono::high_resolution_clock::now();
		std::cout << "Acceptance test #4, 2 allowed loader threads, allow enough memory for all images. \n";
		AcceptanceTest(2, false);

		auto end = std::chrono::high_resolution_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start);

		std::cout << "Acceptance test #4 elapsed time:" << elapsed << " : press enter to continue" << "\n";
		auto wait = std::cin.get();
	}

	std::cout << "All tests completed" << "\n";
	auto wait = std::cin.get();
	return 0;
}

