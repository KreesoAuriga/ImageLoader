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

	cout << "All tests completed" << endl;
	auto wait = cin.get();
	return 0;
}
