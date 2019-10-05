//------------------------------------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/AABB2.hpp"

struct Matrix44;

//------------------------------------------------------------------------------------------------------------------------------
struct AABB3
{
public:
	AABB3();
	~AABB3();
	explicit AABB3(const Vec3& minBounds,const Vec3& maxBounds);

	const static AABB3 UNIT_CUBE;

	void		GetCornersForAABB3(Vec3* corners);

	void		TranslatePointsBy(const Vec3& translation);
	void		TransfromUsingMatrix(const Matrix44& translation);

public:
	//Front face
	Vec3					m_frontTopLeft		= Vec3::ZERO;
	Vec3					m_frontTopRight		= Vec3::ZERO;
	Vec3					m_frontBottomLeft	= Vec3::ZERO;
	Vec3					m_frontBottomRight	= Vec3::ZERO;
												
	//Back face									
	Vec3					m_backTopLeft		= Vec3::ZERO;
	Vec3					m_backTopRight		= Vec3::ZERO;
	Vec3					m_backBottomLeft	= Vec3::ZERO;
	Vec3					m_backBottomRight	= Vec3::ZERO;

	Vec3					m_center			= Vec3::ZERO;
};