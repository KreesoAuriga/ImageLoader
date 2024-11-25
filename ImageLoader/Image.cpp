#include "FileLoader.h"
#include <filesystem>
#include <fstream>
#include <iostream>

#include "tiny-image/src/tinyimage.h"

const ImageFileData* Image::LoadFile(const std::string& filePath) const
{

	/*
	if (!std::filesystem::exists(filePath))
		throw std::runtime_error("File not found at specified path");

	auto file = std::ifstream(filePath, std::ios::in | std::ios::binary);
	if (!file) {
		throw std::runtime_error("Failed to load file.");
		*/

	int width, height;
	unsigned char* image = tinyimg_load(filePath.c_str(), &width, &height, TINYIMG_RGB);

	const auto fileData = new ImageFileData(width, height, image);
	return fileData;
}