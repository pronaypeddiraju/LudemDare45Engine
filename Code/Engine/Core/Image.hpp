//------------------------------------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Commons/EngineCommon.hpp"
#include "Engine/Math/IntVec2.hpp"
#include <vector>

struct Rgba;

//------------------------------------------------------------------------------------------------------------------------------
class Image
{
public:
	//Use stbi load from the third party image thing we added to our engine (refer to getTextureFromFile in render context)
	explicit Image(const char* imagePath);
	explicit Image(const Rgba& color, const int width = 1, const int height = 1);
	Image();
	~Image();

	//Accessors
	//A texel is a pixel in an image. A pixel is a pixel of your screen
	const Rgba& 		GetTexelColor(const IntVec2& texelCoordinates) const;
	const Rgba& 		GetTexelColor(int xCoord, int yCood) const;
	const IntVec2&		GetImageDimensions() const;
	const std::string&	GetImageFilePath() const;
	const uint			GetBytesPerPixel() const;
	const void*			GetImageBuffer() const;

	//Mutators
	void				SetTexelColor(int xCoord, int yCoord, const Rgba& setColor);
	void				SetTexelColor(const IntVec2& texelCoordinates, const Rgba& setColor);

	//void				operator=(const Image& copyFrom);				

private:
	std::string			m_imageFilePath = "";
	IntVec2				m_dimensions = IntVec2::ZERO;
	std::vector<Rgba>	m_texelRepository;
	
	//Raw data
	unsigned char*		m_imageRawData = nullptr;
};