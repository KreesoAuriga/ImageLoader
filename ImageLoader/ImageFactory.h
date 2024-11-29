#include "Image.h"

template<typename TImage>
struct IImageFactory
{
	/// <summary>
	/// Constructs an image of type TImage from raw 8 bit rgba image data. 
	/// </summary>
	/// <param name="width">Width in pixels of the image.</param>
	/// <param name="height">Height in pixels of the image.</param>
	/// <param name="path">The path of the image's source data. This and width/height are used as unique identifiers.</param>
	/// <param name="rgbaData">Pointer to the raw image data. This data will be newly constructed for use by the factory, and the
	/// factory is responsible for destruction of this data via delete[].</param>
	/// <returns>New instance of the image type.</returns>
	[[nodiscard]]
	virtual const TImage* ConstructImage(const int width, const int height, const std::filesystem::path& path, const unsigned char* rgbaData) = 0;
};

