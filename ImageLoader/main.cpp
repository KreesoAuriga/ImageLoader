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
	auto imageLoader = new ImageLoader(imageCache);

	const auto testImagePath1 = testDataPath / "@Response_05.bmp";
	const IImage* testImage1 = nullptr;
	bool acquiredTestImage1 = imageLoader->TryGetImage(testImagePath1, testImage1);

	const IImage* testImage1_again = nullptr;
	bool acquiredTestImage1_again = imageLoader->TryGetImage(testImagePath1, testImage1_again);

	assert(testImage1 == testImage1_again);

	cout << "All tests completed" << endl;
	auto wait = cin.get();
	return 0;
}
