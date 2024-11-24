#pragma once
#include <future>
#include <cassert>

/// <summary>
/// Container for file data which includes the size.
/// </summary>
struct FileData
{
	const int Size;
	const char* Data;

	FileData(int size, const char* data)
		: Size(size)
		, Data(data)
	{
		assert(size >= 0, "File size must be non-negative");
		assert(data, "data cannot be null");
	}
};

struct IFileLoader
{
	virtual std::future<const FileData*> LoadFile(const std::string& filePath) const = 0;
};

class FileLoader : public IFileLoader
{
public:
	virtual std::future<const FileData*> LoadFile(const std::string& filePath) const override = 0;
};