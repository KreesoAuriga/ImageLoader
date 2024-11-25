#include "ImageFileLoader.h"
#include "tiny-image/src/tinyimage.h"

const ImageFileData* ImageFileLoader::LoadFile(const std::string& filePath) const
{
	int width, height;
	unsigned char* image = tinyimg::load(filePath.c_str(), width, height, TINYIMG_RGB);

	auto result = new ImageFileData(width, height, image);
	return result;
}