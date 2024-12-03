#include "ImageDataReader.h"

//per documentation from stb, these defines and include should only occur in a single file
#define STB_IMAGE_IMPLEMENTATION
//exclude formats other than png, bmp, jpg, and tga
//Other formats can be supported in the future.
//gif is excluded because stb can load animated gifs and this project is only supporting non-animated images.
#define STBI_NO_PSD
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM  // (.ppm and .pgm)
#include "../stb/stb_image.h"


inline ImageData* ImageDataReader::ReadFile(const std::filesystem::path& filePath) const
{
	const auto filePathStr = filePath.string();

	if (!std::filesystem::exists(filePathStr))
		return nullptr;

	//For now, we'll force 4 channels for consistency.
	//TODO: zoea 25/11/2024 add affordances to allow for handling images without an alpha to not waste space storing an alpha channel.
	int requiredChannelCount = 4;
	int width, height, channelCount;
	unsigned char* image = stbi_load(filePathStr.c_str(), &width, &height, &channelCount, requiredChannelCount);

	auto result = new ImageData(width, height, image);
	return result;
}

inline ImageData::~ImageData()
{
	if(Data)
		free((void*)Data);
}