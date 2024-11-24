#include "FileLoader.h"
#include <filesystem>
#include <fstream>
#include <iostream>

std::future<const FileData*> FileLoader::LoadFile(const std::string& filePath) const 
{
	if (!std::filesystem::exists(filePath))
		throw std::runtime_error("File not found at specified path");

	auto file = std::ifstream(filePath, std::ios::in | std::ios::binary);
	if (!file) {
		throw std::runtime_error("Failed to load file.");


}