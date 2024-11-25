// ImageLoader.cpp : Defines the entry point for the application.
//

#include "ImageLoader.h"
#include <iostream>
#include <filesystem>

#include "UnitTests/ImageFileLoaderTests.h"

using namespace std;

int main()
{
	const auto testDataPath = std::filesystem::path("F:\\dev\\Serato Interview\\ImageLoader\\TestData");


	cout << "Initialize unit tests" << endl;
	UnitTests::UnitTestsSetup::Initialize(testDataPath);
	auto imageFileLoaderTests = new UnitTests::ImageFileLoaderTests();

	const auto testResultMessages = imageFileLoaderTests->LoadFiles();
	for (const auto& message : testResultMessages)
		cout << message << endl;



	auto imageCache = new ImageCaching::ImageCache(64 * 1024 * 1024);
	auto imageLoader = new ImageLoader(imageCache, 4);


	const auto testImagePath1 = testDataPath / "@Response_05.bmp";
	const IImage* testImage1 = nullptr;
	auto acquiredTestImageFuture1 = imageLoader->TryGetImage(testImagePath1);

	const auto testImagePath2 = testDataPath / "@base (1).jpg";
	const IImage* testImage2 = nullptr;
	auto acquiredTestImageFuture2 = imageLoader->TryGetImage(testImagePath2);

	const IImage* testImage1_again = nullptr;
	auto acquiredTestImageFuture1_again = imageLoader->TryGetImage(testImagePath1);

	testImage1 = acquiredTestImageFuture1.get();
	testImage2 = acquiredTestImageFuture2.get();
	testImage1_again = acquiredTestImageFuture1_again.get();

	assert(testImage1 == testImage1_again);
	
	cout << "All tests completed" << endl;
	auto wait = cin.get();
	return 0;
}
