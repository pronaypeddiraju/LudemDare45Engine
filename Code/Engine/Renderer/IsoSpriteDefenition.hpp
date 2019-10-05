//------------------------------------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Math/Vec3.hpp"
#include <vector>

typedef unsigned int uint;

class SpriteDefenition;
class SpriteSheet;
class TextureView;

//------------------------------------------------------------------------------------------------------------------------------
class IsoSpriteDefenition
{
public:
	IsoSpriteDefenition();
	explicit IsoSpriteDefenition(const SpriteDefenition spriteDefenitions[], uint numDefenitions);
	~IsoSpriteDefenition();

	SpriteDefenition&				GetSpriteForLocalDirection(const Vec3& direction) const;

private:
	std::vector<SpriteDefenition*> m_sprites;
	//We have to determine which out of the 8 quadrants the entity is on
	std::vector<Vec3> m_directions;
};