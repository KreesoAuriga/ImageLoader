#include "ImageFileLoader.h"
//#include "tiny-image/src/tinyimage.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

const ImageFileData* ImageFileLoader::LoadFile(const std::filesystem::path& filePath) const
{
	const auto filePathStr = filePath.string();

	int width, height, channelCount;
	//unsigned char* image = tinyimg::load(filePathCstr, width, height, TinyImgColorType::TINYIMG_RGB);
	unsigned char* image = stbi_load(filePathStr.c_str(), &width, &height, &channelCount, 4);

	auto result = new ImageFileData(width, height, image);
	return result;
}