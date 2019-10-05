//------------------------------------------------------------------------------------------------------------------------------
#include "Engine/Math/PhysicsSystem.hpp"
#include "Engine/Core/EventSystems.hpp"
#include "Engine/Core/NamedProperties.hpp"
#include "Engine/Math/Collider2D.hpp"
#include "Engine/Math/CollisionHandler.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RigidBodyBucket.hpp"
#include "Engine/Math/Rigidbody2D.hpp"
#include "Engine/Math/Trigger2D.hpp"
#include "Engine/Math/TriggerBucket.hpp"
#include "Engine/Renderer/Rgba.hpp"

PhysicsSystem* g_physicsSystem = nullptr;

//------------------------------------------------------------------------------------------------------------------------------
PhysicsSystem::PhysicsSystem()
{
	m_rbBucket = new RigidBodyBucket;
	m_triggerBucket = new TriggerBucket;
}

//------------------------------------------------------------------------------------------------------------------------------
PhysicsSystem::~PhysicsSystem()
{

}

//------------------------------------------------------------------------------------------------------------------------------
Rigidbody2D* PhysicsSystem::CreateRigidbody( eSimulationType simulationType )
{
	Rigidbody2D *rigidbody = new Rigidbody2D(this, simulationType);
	return rigidbody;
}

//------------------------------------------------------------------------------------------------------------------------------
Trigger2D* PhysicsSystem::CreateTrigger(eSimulationType simulationType)
{
	Trigger2D *trigger = new Trigger2D(this, simulationType);
	return trigger;
}

//------------------------------------------------------------------------------------------------------------------------------
void PhysicsSystem::AddRigidbodyToVector(Rigidbody2D* rigidbody)
{
	m_rbBucket->m_RbBucket[rigidbody->GetSimulationType()].push_back(rigidbody);
}

//------------------------------------------------------------------------------------------------------------------------------
void PhysicsSystem::AddTriggerToVector(Trigger2D* trigger)
{
	m_triggerBucket->m_triggerBucket[trigger->GetSimulationType()].push_back(trigger);
}

//------------------------------------------------------------------------------------------------------------------------------
void PhysicsSystem::DestroyRigidbody( Rigidbody2D* rigidbody )
{
	delete rigidbody;
	rigidbody = nullptr;
}

//------------------------------------------------------------------------------------------------------------------------------
void PhysicsSystem::SetGravity( const Vec2& gravity )
{
	m_gravity = gravity;
}

//------------------------------------------------------------------------------------------------------------------------------
void PhysicsSystem::CopyTransformsFromObjects()
{
	// reset per frame stuff; 
	// copy all transforms over;
	for(int rigidTypes = 0; rigidTypes < NUM_SIMULATION_TYPES; rigidTypes++)
	{
		int numRigidbodies = static_cast<int>(m_rbBucket->m_RbBucket[rigidTypes].size());

		for(int rigidbodyIndex = 0; rigidbodyIndex < numRigidbodies; rigidbodyIndex++)
		{
			if(m_rbBucket->m_RbBucket[rigidTypes][rigidbodyIndex] != nullptr)
			{
				m_rbBucket->m_RbBucket[rigidTypes][rigidbodyIndex]->m_transform = *m_rbBucket->m_RbBucket[rigidTypes][rigidbodyIndex]->m_object_transform;
			}
		}
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void PhysicsSystem::CopyTransformsToObjects()
{
	// figure out movement, apply to actual game object;
	for(int rigidTypes = 0; rigidTypes < NUM_SIMULATION_TYPES; rigidTypes++)
	{
		int numRigidbodies = static_cast<int>(m_rbBucket->m_RbBucket[rigidTypes].size());

		for(int rigidbodyIndex = 0; rigidbodyIndex < numRigidbodies; rigidbodyIndex++)
		{
			if(m_rbBucket->m_RbBucket[rigidTypes][rigidbodyIndex] != nullptr)
			{
				*m_rbBucket->m_RbBucket[rigidTypes][rigidbodyIndex]->m_object_transform = m_rbBucket->m_RbBucket[rigidTypes][rigidbodyIndex]->m_transform;				
				m_rbBucket->m_RbBucket[rigidTypes][rigidbodyIndex]->m_transform.m_rotation = m_rbBucket->m_RbBucket[rigidTypes][rigidbodyIndex]->m_rotation;
			}
		}
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void PhysicsSystem::Update( float deltaTime )
{
	CopyTransformsFromObjects(); 

	SetAllCollisionsToFalse();

	RunStep( deltaTime );

	CopyTransformsToObjects();  
}

//------------------------------------------------------------------------------------------------------------------------------
void PhysicsSystem::SetAllCollisionsToFalse()
{
	for(int rigidTypes = 0; rigidTypes < NUM_SIMULATION_TYPES; rigidTypes++)
	{
		int numRigidbodies = static_cast<int>(m_rbBucket->m_RbBucket[rigidTypes].size());

		for(int rigidbodyIndex = 0; rigidbodyIndex < numRigidbodies; rigidbodyIndex++)
		{
			if(m_rbBucket->m_RbBucket[rigidTypes][rigidbodyIndex] != nullptr)
			{
				m_rbBucket->m_RbBucket[rigidTypes][rigidbodyIndex]->m_collider->SetCollision(false);
			}
		}
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void PhysicsSystem::UpdateAllCollisions()
{	
	//Check Static vs Static to mark as collided
	CheckStaticVsStaticCollisions();

	//Dynamic vs Static set 
	ResolveDynamicVsStaticCollisions(true);

	//Dynamic vs Dynamic set
	ResolveDynamicVsDynamicCollisions(true);

	//Dynamic vs static set with no resolution
	ResolveDynamicVsStaticCollisions(false);
}

//------------------------------------------------------------------------------------------------------------------------------
void PhysicsSystem::UpdateTriggers()
{
	//Check if any dynamic object has entered/exited trigger
	int numTriggers = (int)m_triggerBucket->m_triggerBucket->size();
	for (int triggerIndex = 0; triggerIndex < numTriggers; triggerIndex++)
	{
		m_triggerBucket->m_triggerBucket[STATIC_SIMULATION][triggerIndex]->Update(m_frameCount);
	}
	
}

//------------------------------------------------------------------------------------------------------------------------------
void PhysicsSystem::PurgeDeletedObjects()
{
	for (int rbTypes = 0; rbTypes < NUM_SIMULATION_TYPES; rbTypes++)
	{
		for (int rbIndex = 0; rbIndex < m_rbBucket->m_RbBucket[rbTypes].size(); rbIndex++)
		{
			if (!m_rbBucket->m_RbBucket[rbTypes][rbIndex]->m_isAlive)
			{
				delete m_rbBucket->m_RbBucket[rbTypes][rbIndex];
				m_rbBucket->m_RbBucket[rbTypes][rbIndex] = nullptr;

				m_rbBucket->m_RbBucket[rbTypes].erase(m_rbBucket->m_RbBucket[rbTypes].begin() + rbIndex);
				rbIndex--;
			}
		}
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void PhysicsSystem::DebugRender( RenderContext* renderContext ) const
{
	DebugRenderRigidBodies(renderContext);
	DebugRenderTriggers(renderContext);
}

//------------------------------------------------------------------------------------------------------------------------------
void PhysicsSystem::DebugRenderRigidBodies(RenderContext* renderContext) const
{
	for (int rbTypes = 0; rbTypes < NUM_SIMULATION_TYPES; rbTypes++)
	{
		int numRigidbodies = static_cast<int>(m_rbBucket->m_RbBucket[rbTypes].size());
		for (int rbIndex = 0; rbIndex < numRigidbodies; rbIndex++)
		{
			//Nullptr check first
			if (m_rbBucket->m_RbBucket[rbTypes][rbIndex] == nullptr)
			{
				continue;
			}

			if (!m_rbBucket->m_RbBucket[rbTypes][rbIndex]->m_isAlive)
			{
				continue;
			}

			eSimulationType simType = m_rbBucket->m_RbBucket[rbTypes][rbIndex]->GetSimulationType();

			switch (simType)
			{
			case TYPE_UNKOWN:
				break;
			case STATIC_SIMULATION:
			{
				if (m_rbBucket->m_RbBucket[rbTypes][rbIndex]->m_collider->m_inCollision)
				{
					m_rbBucket->m_RbBucket[rbTypes][rbIndex]->DebugRender(renderContext, Rgba::MAGENTA);
				}
				else
				{
					m_rbBucket->m_RbBucket[rbTypes][rbIndex]->DebugRender(renderContext, Rgba::YELLOW);
				}
			}
			break;
			case DYNAMIC_SIMULATION:
			{
				if (m_rbBucket->m_RbBucket[rbTypes][rbIndex]->m_collider->m_inCollision)
				{
					m_rbBucket->m_RbBucket[rbTypes][rbIndex]->DebugRender(renderContext, Rgba::RED);
				}
				else
				{
					m_rbBucket->m_RbBucket[rbTypes][rbIndex]->DebugRender(renderContext, Rgba::BLUE);
				}
			}
			break;
			case NUM_SIMULATION_TYPES:
				break;
			default:
				break;
			}
		}
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void PhysicsSystem::DebugRenderTriggers(RenderContext* renderContext) const
{
	for (int triggerTypes = 0; triggerTypes < NUM_SIMULATION_TYPES; triggerTypes++)
	{
		int numTriggers = static_cast<int>(m_triggerBucket->m_triggerBucket[triggerTypes].size());
		for (int triggerIndex = 0; triggerIndex < numTriggers; triggerIndex++)
		{
			//Nullptr check first
			if (m_triggerBucket->m_triggerBucket[triggerTypes][triggerIndex] == nullptr)
			{
				continue;
			}

			eSimulationType simType = m_triggerBucket->m_triggerBucket[triggerTypes][triggerIndex]->GetSimulationType();

			switch (simType)
			{
			case TYPE_UNKOWN:
				break;
			case STATIC_SIMULATION:
			{
				if (m_triggerBucket->m_triggerBucket[triggerTypes][triggerIndex]->m_collider->m_inCollision)
				{
					m_triggerBucket->m_triggerBucket[triggerTypes][triggerIndex]->DebugRender(renderContext, Rgba::WHITE);
				}
				else
				{
					m_triggerBucket->m_triggerBucket[triggerTypes][triggerIndex]->DebugRender(renderContext, Rgba::WHITE);
				}
			}
			break;
			case DYNAMIC_SIMULATION:
			{
				if (m_triggerBucket->m_triggerBucket[triggerTypes][triggerIndex]->m_collider->m_inCollision)
				{
					m_triggerBucket->m_triggerBucket[triggerTypes][triggerIndex]->DebugRender(renderContext, Rgba::WHITE);
				}
				else
				{
					m_triggerBucket->m_triggerBucket[triggerTypes][triggerIndex]->DebugRender(renderContext, Rgba::WHITE);
				}
			}
			break;
			case NUM_SIMULATION_TYPES:
				break;
			default:
				break;
			}
		}
	}
}

//------------------------------------------------------------------------------------------------------------------------------
const Vec2& PhysicsSystem::GetGravity() const
{
	return m_gravity;
}

//------------------------------------------------------------------------------------------------------------------------------
void PhysicsSystem::RunStep( float deltaTime )
{
	m_frameCount++;

	//First move all rigidbodies based on forces on them
	MoveAllDynamicObjects(deltaTime);

	UpdateAllCollisions();

	UpdateTriggers();
}

//------------------------------------------------------------------------------------------------------------------------------
void PhysicsSystem::MoveAllDynamicObjects(float deltaTime)
{
	int numObjects = static_cast<int>(m_rbBucket->m_RbBucket[DYNAMIC_SIMULATION].size());

	for (int objectIndex = 0; objectIndex < numObjects; objectIndex++)
	{
		if(m_rbBucket->m_RbBucket[DYNAMIC_SIMULATION][objectIndex] != nullptr)
		{
			m_rbBucket->m_RbBucket[DYNAMIC_SIMULATION][objectIndex]->Move(deltaTime);
		}

	}
}

//------------------------------------------------------------------------------------------------------------------------------
void PhysicsSystem::CheckStaticVsStaticCollisions()
{
	int numStaticObjects = static_cast<int>(m_rbBucket->m_RbBucket[STATIC_SIMULATION].size());

	//Set colliding or not colliding here
	for(int colliderIndex = 0; colliderIndex < numStaticObjects; colliderIndex++)
	{
		if(m_rbBucket->m_RbBucket[STATIC_SIMULATION][colliderIndex] == nullptr)
		{
			continue;
		}

		if (!m_rbBucket->m_RbBucket[STATIC_SIMULATION][colliderIndex]->m_isAlive)
		{
			continue;
		}

		for(int otherColliderIndex = 0; otherColliderIndex < numStaticObjects; otherColliderIndex++)
		{
			//check condition where the other collider is nullptr
			if(m_rbBucket->m_RbBucket[STATIC_SIMULATION][otherColliderIndex] == nullptr)
			{
				continue;
			}

			if (!m_rbBucket->m_RbBucket[STATIC_SIMULATION][otherColliderIndex]->m_isAlive)
			{
				continue;
			}

			//Make sure we aren't the same rigidbody
			if(m_rbBucket->m_RbBucket[STATIC_SIMULATION][colliderIndex] == m_rbBucket->m_RbBucket[STATIC_SIMULATION][otherColliderIndex])
			{
				continue;
			}

			Collision2D collision;
			if(m_rbBucket->m_RbBucket[STATIC_SIMULATION][colliderIndex]->m_collider->IsTouching(&collision, m_rbBucket->m_RbBucket[STATIC_SIMULATION][otherColliderIndex]->m_collider))
			{
				//Set collision to true
				m_rbBucket->m_RbBucket[STATIC_SIMULATION][colliderIndex]->m_collider->SetCollision(true);
				m_rbBucket->m_RbBucket[STATIC_SIMULATION][otherColliderIndex]->m_collider->SetCollision(true);

				//Call required collision events
				NamedProperties args;
				m_rbBucket->m_RbBucket[STATIC_SIMULATION][colliderIndex]->m_collider->FireCollisionEvent(args);
			}
		}
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void PhysicsSystem::ResolveDynamicVsStaticCollisions(bool canResolve)
{
	int numDynamicObjects = static_cast<int>(m_rbBucket->m_RbBucket[DYNAMIC_SIMULATION].size());
	int numStaticObjects = static_cast<int>(m_rbBucket->m_RbBucket[STATIC_SIMULATION].size());

	//Set colliding or not colliding here
	for(int colliderIndex = 0; colliderIndex < numDynamicObjects; colliderIndex++)
	{
		if(m_rbBucket->m_RbBucket[DYNAMIC_SIMULATION][colliderIndex] == nullptr)
		{
			continue;
		}

		if (!m_rbBucket->m_RbBucket[DYNAMIC_SIMULATION][colliderIndex]->m_isAlive)
		{
			continue;
		}

		for(int otherColliderIndex = 0; otherColliderIndex < numStaticObjects; otherColliderIndex++)
		{
			//check condition where the other collider is nullptr
			if(m_rbBucket->m_RbBucket[STATIC_SIMULATION][otherColliderIndex] == nullptr)
			{
				continue;
			}

			if (!m_rbBucket->m_RbBucket[STATIC_SIMULATION][otherColliderIndex]->m_isAlive)
			{
				continue;
			}

			Collision2D collision;
			if(m_rbBucket->m_RbBucket[DYNAMIC_SIMULATION][colliderIndex]->m_collider->IsTouching(&collision, m_rbBucket->m_RbBucket[STATIC_SIMULATION][otherColliderIndex]->m_collider))
			{
				//Set collision to true
				m_rbBucket->m_RbBucket[DYNAMIC_SIMULATION][colliderIndex]->m_collider->SetCollision(true);
				m_rbBucket->m_RbBucket[STATIC_SIMULATION][otherColliderIndex]->m_collider->SetCollision(true);

				//Call the collision event
				NamedProperties args;
				m_rbBucket->m_RbBucket[DYNAMIC_SIMULATION][colliderIndex]->m_collider->FireCollisionEvent(args);
				m_rbBucket->m_RbBucket[STATIC_SIMULATION][otherColliderIndex]->m_collider->FireCollisionEvent(args);

				//Push the object out based on the collision manifold
				if(collision.m_manifold.m_normal != Vec2::ZERO)
				{
					m_rbBucket->m_RbBucket[DYNAMIC_SIMULATION][colliderIndex]->m_transform.m_position += collision.m_manifold.m_normal * collision.m_manifold.m_penetration;
				}


				if(canResolve)
				{

					Rigidbody2D* rb0 = m_rbBucket->m_RbBucket[DYNAMIC_SIMULATION][colliderIndex];
					Rigidbody2D* rb1 = m_rbBucket->m_RbBucket[STATIC_SIMULATION][otherColliderIndex];

					Vec2 velocity0 = rb0->m_velocity;
					Vec2 velocity1 = rb1->m_velocity;

					float mass0 = rb0->m_mass; 
				
					Manifold2D manifold = collision.m_manifold;
					Vec2 contactPoint = manifold.m_contact + manifold.m_normal * (manifold.m_penetration);

					//Get the vector from the object centre to the point of contact for both objects
					Vec2 rb0toContact = contactPoint - rb0->m_object_transform->m_position;
					Vec2 rb1toContact = contactPoint - rb1->m_object_transform->m_position;

					//Get the perpendicular of the vector from center to point
					Vec2 toPointPerpendicular0 = rb0toContact.GetRotated90Degrees();
					Vec2 toPointPerpendicular1 = rb1toContact.GetRotated90Degrees();

					//Get the velocity at the impact point for both objects
					Vec2 velocityAtPoint0 = velocity0 + DegreesToRadians(rb0->m_angularVelocity) * toPointPerpendicular0;
					Vec2 velocityAtPoint1 = velocity1 + DegreesToRadians(rb1->m_angularVelocity) * toPointPerpendicular1;

					//Coefficient of restitution
					float CoefficientOfRestitution = (collision.m_Obj->m_rigidbody->m_material.restitution) * (collision.m_otherObj->m_rigidbody->m_material.restitution);
					
					//Generate Impulse along the normal
					float j = -(1 + CoefficientOfRestitution) * GetDotProduct((velocityAtPoint0 - velocityAtPoint1), manifold.m_normal);
					float constant0 = ( GetDotProduct(toPointPerpendicular0, manifold.m_normal) * GetDotProduct(toPointPerpendicular0, manifold.m_normal) / rb0->m_momentOfInertia ) ;
					float d = (1 / mass0) + (constant0);

					float impulseAlongNormal = j / d;

					rb0->ApplyImpulseAt( impulseAlongNormal * collision.m_manifold.m_normal, contactPoint );					

					//Get updated velocity
					velocity0 = rb0->m_velocity;
					velocity1 = rb1->m_velocity;

					//Get the velocity at the impact point for both objects
					velocityAtPoint0 = velocity0 + DegreesToRadians(rb0->m_angularVelocity) * toPointPerpendicular0;
					velocityAtPoint1 = velocity1 + DegreesToRadians(rb1->m_angularVelocity) * toPointPerpendicular1;

					//Generate the impuse along the tangent
					Vec2 tangent = manifold.m_normal.GetRotated90Degrees();
					float jT = -(1 + CoefficientOfRestitution) * GetDotProduct((velocityAtPoint0 - velocityAtPoint1), tangent);
					float constant0T = (GetDotProduct(toPointPerpendicular0, tangent) * GetDotProduct(toPointPerpendicular0, tangent) / rb0->m_momentOfInertia);
					float dT = (1 / mass0) + (constant0T);

					float impulseAlongTangent = jT / dT;

					//Coulumb's law
					float frictionCoefficient = sqrt(abs(rb0->m_friction * rb1->m_friction));

					impulseAlongTangent = Clamp(impulseAlongTangent, -impulseAlongNormal, impulseAlongNormal);
					impulseAlongTangent *= frictionCoefficient;

					rb0->ApplyImpulseAt(impulseAlongTangent * tangent, contactPoint);
				}

			}
		}
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void PhysicsSystem::ResolveDynamicVsDynamicCollisions(bool canResolve)
{
	int numDynamicObjects = static_cast<int>(m_rbBucket->m_RbBucket[DYNAMIC_SIMULATION].size());

	//Set colliding or not colliding here
	for(int colliderIndex = 0; colliderIndex < numDynamicObjects; colliderIndex++)
	{
		if(m_rbBucket->m_RbBucket[DYNAMIC_SIMULATION][colliderIndex] == nullptr)
		{
			continue;
		}

		if (!m_rbBucket->m_RbBucket[DYNAMIC_SIMULATION][colliderIndex]->m_isAlive)
		{
			continue;
		}

		for(int otherColliderIndex = colliderIndex + 1; otherColliderIndex < numDynamicObjects; otherColliderIndex++)
		{
			//check condition where the other collider is nullptr
			if(m_rbBucket->m_RbBucket[DYNAMIC_SIMULATION][otherColliderIndex] == nullptr)
			{
				continue;
			}

			if (!m_rbBucket->m_RbBucket[DYNAMIC_SIMULATION][otherColliderIndex]->m_isAlive)
			{
				continue;
			}

			Collision2D collision;
			if(m_rbBucket->m_RbBucket[DYNAMIC_SIMULATION][colliderIndex]->m_collider->IsTouching(&collision, m_rbBucket->m_RbBucket[DYNAMIC_SIMULATION][otherColliderIndex]->m_collider))
			{
				//Set collision to true
				m_rbBucket->m_RbBucket[DYNAMIC_SIMULATION][colliderIndex]->m_collider->SetCollision(true);
				m_rbBucket->m_RbBucket[DYNAMIC_SIMULATION][otherColliderIndex]->m_collider->SetCollision(true);

				//Push the object out based on the collision manifold
				if(collision.m_manifold.m_normal != Vec2::ZERO)
				{
					float mass0 = m_rbBucket->m_RbBucket[DYNAMIC_SIMULATION][colliderIndex]->m_mass; 
					float mass1 = m_rbBucket->m_RbBucket[DYNAMIC_SIMULATION][otherColliderIndex]->m_mass; 
					float totalMass = mass0 + mass1;

					//Correction on system mass
					float correct0 = mass1 / totalMass;   // move myself along the correction normal
					float correct1 = 1 - correct0;  // move opposite along the normal

					Vec2 move0 = collision.m_manifold.m_normal * collision.m_manifold.m_penetration * correct0;
					Vec2 move1 = (collision.m_manifold.m_normal * -1) * collision.m_manifold.m_penetration * correct1;

					//m_rbBucket->m_RbBucket[DYNAMIC_SIMULATION][colliderIndex]->m_transform.m_position += collision.m_manifold.m_normal * collision.m_manifold.m_penetration * correct0;
					//m_rbBucket->m_RbBucket[DYNAMIC_SIMULATION][otherColliderIndex]->m_transform.m_position += (collision.m_manifold.m_normal * -1) * collision.m_manifold.m_penetration * correct1;
					
					m_rbBucket->m_RbBucket[DYNAMIC_SIMULATION][colliderIndex]->MoveBy(move0);
					m_rbBucket->m_RbBucket[DYNAMIC_SIMULATION][otherColliderIndex]->MoveBy(move1);
				}


				if(canResolve)
				{
					//resolve
					Rigidbody2D* rb0 = m_rbBucket->m_RbBucket[DYNAMIC_SIMULATION][colliderIndex];
					Rigidbody2D* rb1 = m_rbBucket->m_RbBucket[DYNAMIC_SIMULATION][otherColliderIndex];

					//Vec2 *contactPoint = new Vec2();
					//float impulseAlongNormal = GetImpulseAlongNormal(contactPoint, collision, *rb0, *rb1);

					Vec2 velocity0 = rb0->m_velocity;
					Vec2 velocity1 = rb1->m_velocity;

					float mass0 = rb0->m_mass;
					float mass1 = rb1->m_mass;
					float totalMass = mass0 + mass1;

					//Correction on system mass
					float correct0 = mass1 / totalMass;   // move myself along the correction normal
					//float correct1 = 1 - correct0;  // move opposite along the normal

					Manifold2D manifold = collision.m_manifold;
					Vec2 contactPoint = manifold.m_contact + manifold.m_normal * (manifold.m_penetration * correct0);

					//Get the vector from the object centre to the point of contact for both objects
					Vec2 rb0toContact = contactPoint - rb0->m_object_transform->m_position;
					Vec2 rb1toContact = contactPoint - rb1->m_object_transform->m_position;

					//Get the perpendicular of the vector from center to point
					Vec2 toPointPerpendicular0 = rb0toContact.GetRotated90Degrees();
					Vec2 toPointPerpendicular1 = rb1toContact.GetRotated90Degrees();

					//Get the velocity at the impact point for both objects
					Vec2 velocityAtPoint0 = velocity0 + DegreesToRadians(rb0->m_angularVelocity) * toPointPerpendicular0;
					Vec2 velocityAtPoint1 = velocity1 + DegreesToRadians(rb1->m_angularVelocity) * toPointPerpendicular1;

					//Coefficient of restitution
					float CoefficientOfRestitution = (collision.m_Obj->m_rigidbody->m_material.restitution) * (collision.m_otherObj->m_rigidbody->m_material.restitution);

					//Impulse along the normal
					float j = -(1 + CoefficientOfRestitution) * GetDotProduct((velocityAtPoint0 - velocityAtPoint1), manifold.m_normal);
					float constant0 = (GetDotProduct(toPointPerpendicular0, manifold.m_normal) * GetDotProduct(toPointPerpendicular0, manifold.m_normal) / rb0->m_momentOfInertia);
					float constant1 = (GetDotProduct(toPointPerpendicular1, manifold.m_normal) * GetDotProduct(toPointPerpendicular1, manifold.m_normal) / rb1->m_momentOfInertia);
					float d = ((mass0 + mass1) / (mass0 * mass1)) + constant0 + constant1;

					float impulseAlongNormal = j / d;

					rb0->ApplyImpulseAt(impulseAlongNormal * collision.m_manifold.m_normal, contactPoint);
					rb1->ApplyImpulseAt(-1.f * (impulseAlongNormal * collision.m_manifold.m_normal), contactPoint);

					// Get updated velocities
					velocity0 = rb0->m_velocity;
					velocity1 = rb1->m_velocity;

					//Get the velocity at the impact point for both objects
					velocityAtPoint0 = velocity0 + DegreesToRadians(rb0->m_angularVelocity) * toPointPerpendicular0;
					velocityAtPoint1 = velocity1 + DegreesToRadians(rb1->m_angularVelocity) * toPointPerpendicular1;

					//Impulse along the tangent
					Vec2 tangent = manifold.m_normal.GetRotated90Degrees();
					float jT = -(1 + CoefficientOfRestitution) * GetDotProduct((velocityAtPoint0 - velocityAtPoint1), tangent);
					float constant0T = (GetDotProduct(toPointPerpendicular0, tangent) * GetDotProduct(toPointPerpendicular0, tangent) / rb0->m_momentOfInertia);
					float constant1T = (GetDotProduct(toPointPerpendicular1, tangent) * GetDotProduct(toPointPerpendicular1, tangent) / rb1->m_momentOfInertia);
					float dT = ((mass0 + mass1) / (mass0 * mass1)) + constant0T + constant1T;

					float impulseAlongTangent = jT / dT;

					//Coulumb's law
					float frictionCoefficient = sqrt(abs(rb0->m_friction * rb1->m_friction));

					impulseAlongTangent = Clamp(impulseAlongTangent, -impulseAlongNormal, impulseAlongNormal);
					impulseAlongTangent *= frictionCoefficient;

					rb0->ApplyImpulseAt( impulseAlongTangent * tangent, contactPoint );
					rb1->ApplyImpulseAt( -1.f * (impulseAlongTangent * tangent), contactPoint );
				}

			}
		}
	}
}

//------------------------------------------------------------------------------------------------------------------------------
float PhysicsSystem::GetImpulseAlongNormal(Vec2 *out, const Collision2D& collision, const Rigidbody2D& rb0, const Rigidbody2D& rb1)
{
	Vec2 velocity0 = rb0.m_velocity;
	Vec2 velocity1 = rb1.m_velocity;

	float mass0 = rb0.m_mass; 
	float mass1 = rb1.m_mass; 
	float totalMass = mass0 + mass1;

	//Correction on system mass
	float correct0 = mass1 / totalMass;   // move myself along the correction normal
	//float correct1 = 1 - correct0;  // move opposite along the normal

	Manifold2D manifold = collision.m_manifold;
	Vec2 contactPoint = manifold.m_contact + manifold.m_normal * (manifold.m_penetration * correct0);

	//Make sure we save the contact point to return it
	*out = contactPoint;

	//Get the vector from the object centre to the point of contact for both objects
	Vec2 rb0toContact = contactPoint - rb0.m_object_transform->m_position;
	Vec2 rb1toContact = contactPoint - rb1.m_object_transform->m_position;

	//Get the perpendicular of the vector from center to point
	Vec2 toPointPerpendicular0 = rb0toContact.GetRotated90Degrees();
	Vec2 toPointPerpendicular1 = rb1toContact.GetRotated90Degrees();

	//Generate the impulse along normal

	//Get the velocity at the impact point for both objects
	Vec2 velocityAtPoint0 = velocity0 + DegreesToRadians(rb0.m_angularVelocity) * toPointPerpendicular0;
	Vec2 velocityAtPoint1 = velocity1 + DegreesToRadians(rb1.m_angularVelocity) * toPointPerpendicular1;

	//Coefficient of restitution
	float CoefficientOfRestitution = (collision.m_Obj->m_rigidbody->m_material.restitution) * (collision.m_otherObj->m_rigidbody->m_material.restitution);

	float j = -(1 + CoefficientOfRestitution) * GetDotProduct((velocityAtPoint0 - velocityAtPoint1), manifold.m_normal);

	float constant0 = ( GetDotProduct(toPointPerpendicular0, manifold.m_normal) * GetDotProduct(toPointPerpendicular0, manifold.m_normal) / rb0.m_momentOfInertia ) ;
	float constant1 = ( GetDotProduct(toPointPerpendicular1, manifold.m_normal) * GetDotProduct(toPointPerpendicular1, manifold.m_normal) / rb1.m_momentOfInertia );
	
	float d = ((mass0 + mass1) / (mass0 * mass1)) + constant0 + constant1;

	float impulseAlongNormal = j / d;
	return impulseAlongNormal;
}