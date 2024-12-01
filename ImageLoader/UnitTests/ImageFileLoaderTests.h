#pragma once
#include "UnitTestsSetup.h"
#include "../ImageFileLoader.h"
#include <iostream>
#include "assert.h"

namespace UnitTests
{
	enum TestResult
	{
		Fail,
		Pass,
		Undefined
	};


	class ImageFileLoaderTests
	{

	public:
		TestResult LoadFile(const std::string& fileName, int expectedWidth, int expectedHeight, std::string& outMessage)
		{
			auto loader = new ImageDataReader();

			auto path = UnitTestsSetup::GetTestDataPath();
			path /= fileName;

			const ImageData* loadedFile = loader->ReadFile(path);

			ASSERT(loadedFile);
			ASSERT(loadedFile->Width == expectedWidth);
			ASSERT(loadedFile->Height == expectedHeight);
			ASSERT(loadedFile->Data != nullptr);


			outMessage = "test: LoadFile " + fileName + " passed";
			return TestResult::Pass;
		}

		std::vector<std::string> LoadFiles()
		{
			auto results = std::vector<std::string>();

			std::string testMessage;
			if (LoadFile("@Response_05.bmp", 1920, 1080, testMessage) != UnitTests::TestResult::Undefined)
				results.emplace_back(testMessage);
			else
				results.emplace_back("unknown error");

			if (LoadFile("@base_01.jpg", 1920, 1080, testMessage) != UnitTests::TestResult::Undefined)
				results.emplace_back(testMessage);
			else
				results.emplace_back("unknown error");

			return results;
		}

	};
}