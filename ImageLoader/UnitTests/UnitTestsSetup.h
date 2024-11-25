#pragma once
#include <string>
#include <stdexcept>
#include <filesystem>

namespace UnitTests
{
	class UnitTestsSetup
	{
	private:
		static bool _hasBeenInitialized;
		static std::filesystem::path _testDataPath;

	public:

		static void Initialize(const std::filesystem::path& testDataPath)
		{
			_testDataPath = testDataPath;
			_hasBeenInitialized = true;
		}

		static const std::filesystem::path& GetTestDataPath()
		{
			if (!_hasBeenInitialized)
				throw std::runtime_error("UnitTestsSetup::Initialize(...) has not been called.");

			return _testDataPath;
		}
	};

	bool UnitTestsSetup::_hasBeenInitialized = false;
	std::filesystem::path UnitTestsSetup::_testDataPath = std::filesystem::path("");
}