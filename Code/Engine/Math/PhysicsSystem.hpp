//------------------------------------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Commons/EngineCommon.hpp"
#include "Engine/Math/Rigidbody2D.hpp"

//------------------------------------------------------------------------------------------------------------------------------
class RenderContext;
class Collider2D;
class RigidBodyBucket;
class Trigger2D;
class TriggerBucket;
struct Collision2D;

//------------------------------------------------------------------------------------------------------------------------------
class PhysicsSystem
{
	friend class Rigidbody2D;

public:
	PhysicsSystem();
	~PhysicsSystem();

	Rigidbody2D*			CreateRigidbody(eSimulationType simulationType);
	Trigger2D*				CreateTrigger(eSimulationType simulationType);
	void					AddRigidbodyToVector( Rigidbody2D* rigidbody );
	void					AddTriggerToVector(Trigger2D* trigger);
	void					DestroyRigidbody( Rigidbody2D* rigidbody );
	void					SetGravity(const Vec2& gravity);

	void					CopyTransformsFromObjects();
	void					CopyTransformsToObjects();
	void					Update(float deltaTime);
	void					SetAllCollisionsToFalse();
	void					UpdateAllCollisions();
	void					UpdateTriggers();

	void					PurgeDeletedObjects();

	void					DebugRender( RenderContext* renderContext ) const;
	void					DebugRenderRigidBodies( RenderContext* renderContext ) const;
	void					DebugRenderTriggers( RenderContext* renderContext ) const;

	const Vec2&				GetGravity() const;

private:

	void					RunStep(float deltaTime);

	void					MoveAllDynamicObjects(float deltaTime);
	void					CheckStaticVsStaticCollisions();
	void					ResolveDynamicVsStaticCollisions( bool canResolve );
	void					ResolveDynamicVsDynamicCollisions( bool canResolve );

	//Utilities
	float					GetImpulseAlongNormal( Vec2* out, const Collision2D& collision, const Rigidbody2D& rb0, const Rigidbody2D& rb1 );

public:

	//Way to store all rigid-bodies
	RigidBodyBucket*				m_rbBucket;
	TriggerBucket*					m_triggerBucket;
	uint							m_frameCount = 0U;


	//system info like gravity
	Vec2							m_gravity = Vec2(0.0f, -9.8f);
};