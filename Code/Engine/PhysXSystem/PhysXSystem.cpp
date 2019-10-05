//------------------------------------------------------------------------------------------------------------------------------
#include "Engine/PhysXSystem/PhysXSystem.hpp"
#include "Engine/Commons/EngineCommon.hpp"
#include "Engine/Math/Matrix44.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec4.hpp"
//PhysX API
#include "ThirdParty/PhysX/include/PxPhysicsAPI.h"
//Vehicle Includes
#include "Engine/PhysXSystem/PhysXVehicleFilterShader.hpp"
#include "Engine/PhysXSystem/PhysXVehicleTireFriction.hpp"
#include "Engine/PhysXSystem/PhysXVehicleCreate.hpp"
#include "Engine/PhysXSystem/PhysXWheelContactModifyCallback.hpp"
#include "Engine/PhysXSystem/PhysXWheelCCDContactModifyCallback.hpp"

//PhysX Pragma Comments
#if ( defined( _WIN64 ) & defined( _DEBUG ) )
#pragma comment( lib, "ThirdParty/PhysX/lib/debug_x64/PhysX_64.lib" )
#pragma comment( lib, "ThirdParty/PhysX/lib/debug_x64/PhysXCommon_64.lib" )
#pragma comment( lib, "ThirdParty/PhysX/lib/debug_x64/PhysXCooking_64.lib" )
#pragma comment( lib, "ThirdParty/PhysX/lib/debug_x64/PhysXExtensions_static_64.lib" )
#pragma comment( lib, "ThirdParty/PhysX/lib/debug_x64/PhysXFoundation_64.lib" )
#pragma comment( lib, "ThirdParty/PhysX/lib/debug_x64/PhysXPvdSDK_static_64.lib" )
#pragma comment( lib, "ThirdParty/PhysX/lib/debug_x64/PhysXVehicle_static_64.lib" )
#elif ( defined ( _WIN64 ) & defined( NDEBUG ) )
#pragma comment( lib, "ThirdParty/PhysX/lib/release_x64/PhysX_64.lib" )
#pragma comment( lib, "ThirdParty/PhysX/lib/release_x64/PhysXCommon_64.lib" )
#pragma comment( lib, "ThirdParty/PhysX/lib/release_x64/PhysXCooking_64.lib" )
#pragma comment( lib, "ThirdParty/PhysX/lib/release_x64/PhysXExtensions_static_64.lib" )
#pragma comment( lib, "ThirdParty/PhysX/lib/release_x64/PhysXFoundation_64.lib" )
#pragma comment( lib, "ThirdParty/PhysX/lib/release_x64/PhysXPvdSDK_static_64.lib" )
#pragma comment( lib, "ThirdParty/PhysX/lib/release_x64/PhysXVehicle_static_64.lib" )
#elif ( defined( _WIN32 ) & defined( _DEBUG ) )
#pragma comment( lib, "ThirdParty/PhysX/lib/debug_x86/PhysX_32.lib" )
#pragma comment( lib, "ThirdParty/PhysX/lib/debug_x86/PhysXCommon_32.lib" )
#pragma comment( lib, "ThirdParty/PhysX/lib/debug_x86/PhysXCooking_32.lib" )
#pragma comment( lib, "ThirdParty/PhysX/lib/debug_x86/PhysXExtensions_static_32.lib" )
#pragma comment( lib, "ThirdParty/PhysX/lib/debug_x86/PhysXFoundation_32.lib" )
#pragma comment( lib, "ThirdParty/PhysX/lib/debug_x86/PhysXPvdSDK_static_32.lib" )
#pragma comment( lib, "ThirdParty/PhysX/lib/debug_x86/PhysXVehicle_static_32.lib" )
#elif ( defined( _WIN32 ) & defined( NDEBUG ) )
#pragma comment( lib, "ThirdParty/PhysX/lib/release_x86/PhysX_32.lib" )
#pragma comment( lib, "ThirdParty/PhysX/lib/release_x86/PhysXCommon_32.lib" )
#pragma comment( lib, "ThirdParty/PhysX/lib/release_x86/PhysXCooking_32.lib" )
#pragma comment( lib, "ThirdParty/PhysX/lib/release_x86/PhysXExtensions_static_32.lib" )
#pragma comment( lib, "ThirdParty/PhysX/lib/release_x86/PhysXFoundation_32.lib" )
#pragma comment( lib, "ThirdParty/PhysX/lib/release_x86/PhysXPvdSDK_static_32.lib" )
#pragma comment( lib, "ThirdParty/PhysX/lib/release_x86/PhysXVehicle_static_32.lib" )
#endif

//Angle thresholds used to categorize contacts as suspension contacts or rigid body contacts
//Anything above this threshold cannot be a suspension contact
#define POINT_REJECT_ANGLE PxPi/4.0f
#define NORMAL_REJECT_ANGLE PxPi/4.0f

//PhysX Vehicles support blocking and non-blocking sweeps
//Switching this will result in the number of hit queries recieved by wheel to change
//	based on how we set it up
#define BLOCKING_SWEEPS 1

//Define the maximum acceleration for dynamic bodies under the wheel
#define MAX_ACCELERATION 50.0f

//Blocking sweeps require sweep hit buffers for just 1 hit per wheel
//Non-blocking sweeps require more hits per wheel because they return all touches 
//	on the swept shape
#if BLOCKING_SWEEPS
short					gNumQueryHitsPerWheel = 1;
#else
short					gNumQueryHitsPerWheel = 8;
#endif


PhysXSystem* g_PxPhysXSystem = nullptr;

//------------------------------------------------------------------------------------------------------------------------------
PhysXSystem::PhysXSystem()
{
	StartUp();
}

//------------------------------------------------------------------------------------------------------------------------------
PhysXSystem::~PhysXSystem()
{
	ShutDown();
}

//------------------------------------------------------------------------------------------------------------------------------
void PhysXSystem::StartUp()
{
	//PhysX starts off by setting up a Physics Foundation
	m_PxFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, m_PxAllocator, m_PXErrorCallback);

	//Setup PhysX cooking if you need convex hull cooking support
	m_PxCooking = PxCreateCooking(PX_PHYSICS_VERSION, *m_PxFoundation, PxCookingParams(PxTolerancesScale()));

	//Create the PhysX Visual Debugger by giving it the current foundation
	m_Pvd = PxCreatePvd(*m_PxFoundation);
	//The PVD needs connection via a socket. It will run on the Address defined, in our case it's our machine
	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(m_pvdIPAddress.c_str(), m_pvdPortNumber, m_pvdTimeOutSeconds);
	m_Pvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

	//Create Physics! This creates an instance of the PhysX SDK
	m_PhysX = PxCreatePhysics(PX_PHYSICS_VERSION, *m_PxFoundation, PxTolerancesScale(), true, m_Pvd);
	PxInitExtensions(*m_PhysX, m_Pvd);

	//What is the description of this PhysX scene?
	PxSceneDesc sceneDesc(m_PhysX->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	//This creates CPU dispatcher threads or worker threads. We will make 2
	m_PxDispatcher = PxDefaultCpuDispatcherCreate(2);
	sceneDesc.cpuDispatcher = m_PxDispatcher;
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	//Create the scene now by passing the scene's description
	m_PxScene = m_PhysX->createScene(sceneDesc);

	PxPvdSceneClient* pvdClient = m_PxScene->getScenePvdClient();
	if (pvdClient)
	{
		//I have a PVD client, so set some flags that it needs
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}

	m_PxDefaultMaterial = m_PhysX->createMaterial(m_defaultStaticFriction, m_defaultDynamicFriction, m_defaultRestitution);
}

//------------------------------------------------------------------------------------------------------------------------------
PxVehicleDrive4W* PhysXSystem::StartUpVehicleSDK()
{
	//------------------------------------------------------------------------------------------------------------------------------
	// Vehicle SDK Setup
	//------------------------------------------------------------------------------------------------------------------------------
	m_vehicleKitEnabled = true;

	m_PxScene->release();
	m_PxScene = nullptr;
	PxSceneDesc sceneDesc(m_PhysX->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	m_PxDispatcher = PxDefaultCpuDispatcherCreate(1);
	sceneDesc.cpuDispatcher = m_PxDispatcher;
	//sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	sceneDesc.filterShader = VehicleFilterShader;
	sceneDesc.contactModifyCallback = &gWheelContactModifyCallback;			//Enable contact modification
	sceneDesc.ccdContactModifyCallback = &gWheelCCDContactModifyCallback;	//Enable ccd contact modification
	sceneDesc.flags |= PxSceneFlag::eENABLE_CCD;							//Enable ccd
	m_PxScene = m_PhysX->createScene(sceneDesc);

	PxInitVehicleSDK(*m_PhysX);
	PxVehicleSetBasisVectors(PxVec3(0, 1, 0), PxVec3(0, 0, 1));
	PxVehicleSetUpdateMode(PxVehicleUpdateMode::eVELOCITY_CHANGE);
	PxVehicleSetSweepHitRejectionAngles(POINT_REJECT_ANGLE, NORMAL_REJECT_ANGLE);
	PxVehicleSetMaxHitActorAcceleration(MAX_ACCELERATION);

	//Create the batched scene queries for the suspension sweeps.
	//Use the post-filter shader to reject hit shapes that overlap the swept wheel at the start pose of the sweep.
	PxQueryHitType::Enum(*sceneQueryPreFilter)(PxFilterData, PxFilterData, const void*, PxU32, PxHitFlags&);
	PxQueryHitType::Enum(*sceneQueryPostFilter)(PxFilterData, PxFilterData, const void*, PxU32, const PxQueryHit&);
#if BLOCKING_SWEEPS
	sceneQueryPreFilter = &WheelSceneQueryPreFilterBlocking;
	sceneQueryPostFilter = &WheelSceneQueryPostFilterBlocking;
#else
	sceneQueryPreFilter = &WheelSceneQueryPreFilterNonBlocking;
	sceneQueryPostFilter = &WheelSceneQueryPostFilterNonBlocking;
#endif 

	//Create the batched scene queries for the suspension raycasts.
	m_vehicleSceneQueryData = VehicleSceneQueryData::allocate(m_numberOfVehicles, PX_MAX_NB_WHEELS, gNumQueryHitsPerWheel, m_numberOfVehicles, sceneQueryPreFilter, sceneQueryPostFilter, m_PxAllocator);
	m_batchQuery = VehicleSceneQueryData::setUpBatchedSceneQuery(0, *m_vehicleSceneQueryData, m_PxScene);

	//Create the friction table for each combination of tire and surface type.
	m_frictionPairs = createFrictionPairs(m_PxDefaultMaterial);

	//Create a plane to drive on.
	PxFilterData groundPlaneSimFilterData(COLLISION_FLAG_GROUND, COLLISION_FLAG_GROUND_AGAINST, 0, 0);
	m_drivableGroundPlane = createDrivablePlane(groundPlaneSimFilterData, m_PxDefaultMaterial, m_PhysX);
	m_PxScene->addActor(*m_drivableGroundPlane);
	
	//Create a vehicle that will drive on the plane.
	PxFilterData chassisSimFilterData(COLLISION_FLAG_CHASSIS, COLLISION_FLAG_GROUND, 0, 0);
	PxFilterData wheelSimFilterData(COLLISION_FLAG_WHEEL, COLLISION_FLAG_WHEEL, PxPairFlag::eDETECT_CCD_CONTACT | PxPairFlag::eMODIFY_CONTACTS, 0);
	VehicleDesc vehicleDesc = InitializeVehicleDescription(chassisSimFilterData, wheelSimFilterData);
	PxVehicleDrive4W* vehicleReference = createVehicle4W(vehicleDesc, m_PhysX, m_PxCooking);
	PxTransform startTransform(PxVec3(0, (vehicleDesc.chassisDims.y*0.5f + vehicleDesc.wheelRadius + 1.0f), 0), PxQuat(PxIdentity));
	vehicleReference->getRigidDynamicActor()->setGlobalPose(startTransform);
	vehicleReference->getRigidDynamicActor()->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true);
	m_PxScene->addActor(*vehicleReference->getRigidDynamicActor());

	//Set the vehicle to rest in first gear.
	//Set the vehicle to use auto-gears.
	vehicleReference->setToRestState();
	vehicleReference->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
	vehicleReference->mDriveDynData.setUseAutoGears(true);

	//Return the reference to the car that was created
	return vehicleReference;
}

//------------------------------------------------------------------------------------------------------------------------------
VehicleDesc PhysXSystem::InitializeVehicleDescription(const PxFilterData& chassisSimFilterData, const PxFilterData& wheelSimFilterData)
{
	//Set up the chassis mass, dimensions, moment of inertia, and center of mass offset.
	//The moment of inertia is just the moment of inertia of a cuboid but modified for easier steering.
	//Center of mass offset is 0.65m above the base of the chassis and 0.25m towards the front.
	const PxF32 chassisMass = 1500.0f;
	const PxVec3 chassisDims(2.5f, 2.0f, 5.0f);
	const PxVec3 chassisMOI
	((chassisDims.y*chassisDims.y + chassisDims.z*chassisDims.z)*chassisMass / 12.0f,
		(chassisDims.x*chassisDims.x + chassisDims.z*chassisDims.z)*0.8f*chassisMass / 12.0f,
		(chassisDims.x*chassisDims.x + chassisDims.y*chassisDims.y)*chassisMass / 12.0f);
	const PxVec3 chassisCMOffset(0.0f, -chassisDims.y*0.5f + 0.65f, 0.25f);

	//Set up the wheel mass, radius, width, moment of inertia, and number of wheels.
	//Moment of inertia is just the moment of inertia of a cylinder.
	const PxF32 wheelMass = 20.0f;
	const PxF32 wheelRadius = 0.5f;
	const PxF32 wheelWidth = 0.4f;
	const PxF32 wheelMOI = 0.5f*wheelMass*wheelRadius*wheelRadius;
	const PxU32 nbWheels = 4;

	VehicleDesc vehicleDesc;

	vehicleDesc.chassisMass = chassisMass;
	vehicleDesc.chassisDims = chassisDims;
	vehicleDesc.chassisMOI = chassisMOI;
	vehicleDesc.chassisCMOffset = chassisCMOffset;
	vehicleDesc.chassisMaterial = m_PxDefaultMaterial;
	vehicleDesc.chassisSimFilterData = chassisSimFilterData;

	vehicleDesc.wheelMass = wheelMass;
	vehicleDesc.wheelRadius = wheelRadius;
	vehicleDesc.wheelWidth = wheelWidth;
	vehicleDesc.wheelMOI = wheelMOI;
	vehicleDesc.numWheels = nbWheels;
	vehicleDesc.wheelMaterial = m_PxDefaultMaterial;
	vehicleDesc.chassisSimFilterData = wheelSimFilterData;

	vehicleDesc.actorUserData = &m_actorUserData;
	vehicleDesc.shapeUserDatas = m_shapeUserData;

	return vehicleDesc;
}

//------------------------------------------------------------------------------------------------------------------------------
void PhysXSystem::BeginFrame()
{

}

//------------------------------------------------------------------------------------------------------------------------------
void PhysXSystem::Update(float deltaTime)
{
	m_PxScene->simulate(deltaTime);
	m_PxScene->fetchResults(true);
}

//------------------------------------------------------------------------------------------------------------------------------
void PhysXSystem::EndFrame()
{

}

//------------------------------------------------------------------------------------------------------------------------------
physx::PxScene* PhysXSystem::GetPhysXScene() const
{
	return m_PxScene;
}

//------------------------------------------------------------------------------------------------------------------------------
physx::PxPhysics* PhysXSystem::GetPhysXSDK() const
{
	return m_PhysX;
}

//------------------------------------------------------------------------------------------------------------------------------
physx::PxCooking* PhysXSystem::GetPhysXCookingModule() const
{
	return m_PxCooking;
}

//------------------------------------------------------------------------------------------------------------------------------
physx::PxFoundation* PhysXSystem::GetPhysXFoundationModule() const
{
	return m_PxFoundation; 
}

//------------------------------------------------------------------------------------------------------------------------------
vehicle::VehicleSceneQueryData* PhysXSystem::GetVehicleSceneQueryData() const
{
	return m_vehicleSceneQueryData;
}

//------------------------------------------------------------------------------------------------------------------------------
physx::PxVehicleDrivableSurfaceToTireFrictionPairs* PhysXSystem::GetVehicleTireFrictionPairs() const
{
	return m_frictionPairs;
}

//------------------------------------------------------------------------------------------------------------------------------
physx::PxBatchQuery* PhysXSystem::GetPhysXBatchQuery() const
{
	return m_batchQuery;
}

//------------------------------------------------------------------------------------------------------------------------------
physx::PxRigidDynamic* PhysXSystem::CreateDynamicObject(const PxGeometry& pxGeometry, const Vec3& velocity, const Matrix44& matrix, float materialDensity)
{
	if (materialDensity < 0.f)
	{
		materialDensity = m_defaultDensity;
	}

	PxPhysics* physX = g_PxPhysXSystem->GetPhysXSDK();
	PxScene* pxScene = g_PxPhysXSystem->GetPhysXScene();

	PxVec3 pxVelocity = PxVec3(velocity.x, velocity.y, velocity.z);
	Vec3 position = matrix.GetTBasis();
	PxVec3 pxPosition = VecToPxVector(position);

	PxTransform pxTransform(pxPosition);
	pxTransform.q = MakeQuaternionFromMatrix(matrix);

	PxRigidDynamic* dynamic = PxCreateDynamic(*physX, pxTransform, pxGeometry, *m_PxDefaultMaterial, materialDensity);
    dynamic->setAngularDamping(m_defaultAngularDamping);
	dynamic->setLinearVelocity(pxVelocity);
	pxScene->addActor(*dynamic);

	return dynamic;
}

//------------------------------------------------------------------------------------------------------------------------------
void PhysXSystem::CreateRandomConvexHull(std::vector<Vec3>& vertexArray, int gaussMapLimit, bool directInsertion)
{
	PxCooking* pxCookingModule = g_PxPhysXSystem->GetPhysXCookingModule();
	PxPhysics* pxPhysics = g_PxPhysXSystem->GetPhysXSDK();
	PxScene* pxScene = g_PxPhysXSystem->GetPhysXScene();
	PxCookingParams params = pxCookingModule->getParams();

	// Use the new (default) PxConvexMeshCookingType::eQUICKHULL
	params.convexMeshCookingType = g_PxPhysXSystem->GetPxConvexMeshCookingType(QUICKHULL);

	// If the gaussMapLimit is chosen higher than the number of output vertices, no gauss map is added to the convex mesh data (here 256).
	// If the gaussMapLimit is chosen lower than the number of output vertices, a gauss map is added to the convex mesh data (here 16).
	params.gaussMapLimit = gaussMapLimit;
	pxCookingModule->setParams(params);

	// Setup the convex mesh descriptor
	PxConvexMeshDesc desc;
	PxConvexMesh* pxConvexMesh;

	std::vector<PxVec3> pxVecArray;
	pxVecArray.reserve(vertexArray.size());

	int numVerts = (int)vertexArray.size();
	for (int vecIndex = 0; vecIndex < numVerts; vecIndex++)
	{
		pxVecArray.push_back(g_PxPhysXSystem->VecToPxVector(vertexArray[vecIndex]));
	}

	// We provide points only, therefore the PxConvexFlag::eCOMPUTE_CONVEX flag must be specified
	desc.points.data = &pxVecArray[0];
	desc.points.count = numVerts;
	desc.points.stride = sizeof(PxVec3);
	desc.flags = PxConvexFlag::eCOMPUTE_CONVEX;

	PxU32 meshSize = 0;

	if (directInsertion)
	{
		// Directly insert mesh into PhysX
		pxConvexMesh = pxCookingModule->createConvexMesh(desc, pxPhysics->getPhysicsInsertionCallback());
		PX_ASSERT(convex);
	}
	else
	{
		// Serialize the cooked mesh into a stream.
		PxFoundation* pxFoundation = g_PxPhysXSystem->GetPhysXFoundationModule();
		PxDefaultMemoryOutputStream outStream(pxFoundation->getAllocatorCallback());
		bool res = pxCookingModule->cookConvexMesh(desc, outStream);
		PX_UNUSED(res);
		PX_ASSERT(res);
		meshSize = outStream.getSize();

		// Create the mesh from a stream.
		PxDefaultMemoryInputData inStream(outStream.getData(), outStream.getSize());
		pxConvexMesh = pxPhysics->createConvexMesh(inStream);

		//Make the geometrue
		PxConvexMeshGeometry geometry = PxConvexMeshGeometry(pxConvexMesh);
		const PxMaterial* material = g_PxPhysXSystem->GetDefaultPxMaterial();

		PxShape* shape = pxPhysics->createShape(geometry, *material);

		/*
		PxFilterData groundPlaneSimFilterData(COLLISION_FLAG_GROUND, COLLISION_FLAG_GROUND_AGAINST, 0, 0);
		PxFilterData qryFilterData;
		setupDrivableSurface(qryFilterData);
		shape->setQueryFilterData(qryFilterData);
		shape->setSimulationFilterData(groundPlaneSimFilterData);
		*/

		PxTransform localTm(PxVec3(0, 50.f, 0));
		PxRigidDynamic* body = pxPhysics->createRigidDynamic(localTm);
		body->attachShape(*shape);
		PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
		pxScene->addActor(*body);

		PX_ASSERT(convex);
	}

	pxConvexMesh->release();
}

//------------------------------------------------------------------------------------------------------------------------------
physx::PxConvexMeshCookingType::Enum PhysXSystem::GetPxConvexMeshCookingType(PhysXConvexMeshCookingTypes_T meshType)
{
	switch (meshType)
	{
	case QUICKHULL:
	{
		return PxConvexMeshCookingType::eQUICKHULL;
	}
		break;
	default:
	{
		ERROR_AND_DIE("Mesh Type is not supported by PhysX");
	}
		break;
	}
}

//------------------------------------------------------------------------------------------------------------------------------
// NOTE: Interface asks for angles in degrees but PhysX needs angles in Radians
//------------------------------------------------------------------------------------------------------------------------------
physx::PxJoint* PhysXSystem::CreateJointLimitedSpherical(PxRigidActor* actorA, const Vec3& positionA, PxRigidActor* actorB, const Vec3& positionB, float yAngleLimit, float zAngleLimit, float contactDistance)
{
	PxTransform transformA(VecToPxVector(positionA));
	PxTransform transformB(VecToPxVector(positionB));

	PxSphericalJoint* joint = PxSphericalJointCreate(*m_PhysX, actorA, transformA, actorB, transformB);
	joint->setLimitCone(PxJointLimitCone(DegreesToRadians(yAngleLimit), DegreesToRadians(zAngleLimit), contactDistance));
	joint->setSphericalJointFlag(PxSphericalJointFlag::eLIMIT_ENABLED, true);
	joint->setConstraintFlag(PxConstraintFlag::ePROJECTION, true);
	return joint;
}

//------------------------------------------------------------------------------------------------------------------------------
// Unbreakable Fixed Joint
//------------------------------------------------------------------------------------------------------------------------------
physx::PxJoint* PhysXSystem::CreateJointSimpleFixed(PxRigidActor* actorA, const Vec3& positionA, PxRigidActor* actorB, const Vec3& positionB)
{
	PxTransform transformA(VecToPxVector(positionA));
	PxTransform transformB(VecToPxVector(positionB));

	PxFixedJoint* joint = PxFixedJointCreate(*m_PhysX, actorA, transformA, actorB, transformB);
	joint->setBreakForce(FLT_MAX, FLT_MAX);
	joint->setConstraintFlag(PxConstraintFlag::eDRIVE_LIMITS_ARE_FORCES, true);
	joint->setConstraintFlag(PxConstraintFlag::eDISABLE_PREPROCESSING, true);
	return joint;
}

//------------------------------------------------------------------------------------------------------------------------------
// Unrestricted Spherical Joint (Rotation permitted to 180 degrees in y and z)
// NOTE: PhysX needs the angles in Radians, in the simple case we pass it PxHalfPi for y and z cone limits
//------------------------------------------------------------------------------------------------------------------------------
physx::PxJoint* PhysXSystem::CreateJointSimpleSpherical(PxRigidActor* actorA, const Vec3& positionA, PxRigidActor* actorB, const Vec3& positionB)
{
	PxTransform transformA(VecToPxVector(positionA));
	PxTransform transformB(VecToPxVector(positionB));

	PxSphericalJoint* joint = PxSphericalJointCreate(*m_PhysX, actorA, transformA, actorB, transformB);
	joint->setLimitCone(PxJointLimitCone(PxHalfPi, PxHalfPi));
	joint->setSphericalJointFlag(PxSphericalJointFlag::eLIMIT_ENABLED, true);
	return joint;
}

//------------------------------------------------------------------------------------------------------------------------------
// NOTE: This creates a fixed joint which we set a break force to.
// By default the breakable joint is unbreakable(max float for break force)
//------------------------------------------------------------------------------------------------------------------------------
physx::PxJoint* PhysXSystem::CreateJointBreakableFixed(PxRigidActor* actorA, const Vec3& positionA, PxRigidActor* actorB, const Vec3& positionB, float breakForce, float breakTorque)
{
	PxTransform transformA(VecToPxVector(positionA));
	PxTransform transformB(VecToPxVector(positionB));

	PxFixedJoint* joint = PxFixedJointCreate(*m_PhysX, actorA, transformA, actorB, transformB);
	joint->setBreakForce(breakForce, breakTorque);
	joint->setConstraintFlag(PxConstraintFlag::eDRIVE_LIMITS_ARE_FORCES, true);
	joint->setConstraintFlag(PxConstraintFlag::eDISABLE_PREPROCESSING, true);
	return joint;
}

//------------------------------------------------------------------------------------------------------------------------------
// NOTE: By default the max drive force limit is set to FLT_MAX and the drive is force dependent (Not acceleration dependent)
//------------------------------------------------------------------------------------------------------------------------------
physx::PxJoint* PhysXSystem::CreateJointDampedD6(PxRigidActor* actorA, const Vec3& positionA, PxRigidActor* actorB, const Vec3& positionB, float driveStiffness, float driveDamping, float driveForceLimit, bool isDriveAcceleration)
{
	PxTransform transformA(VecToPxVector(positionA));
	PxTransform transformB(VecToPxVector(positionB));

	PxD6Joint* joint = PxD6JointCreate(*m_PhysX, actorA, transformA, actorB, transformB);
	joint->setMotion(PxD6Axis::eSWING1, PxD6Motion::eFREE);
	joint->setMotion(PxD6Axis::eSWING2, PxD6Motion::eFREE);
	joint->setMotion(PxD6Axis::eTWIST, PxD6Motion::eFREE);
	joint->setDrive(PxD6Drive::eSLERP, PxD6JointDrive(driveStiffness, driveDamping, driveForceLimit, isDriveAcceleration));
	return joint;
}

void PhysXSystem::CreateSimpleSphericalChain(const Vec3& position, int length, const PxGeometry& geometry, float separation)
{
	PxTransform transform(PhysXSystem::VecToPxVector(position));
	PxPhysics* physX = g_PxPhysXSystem->GetPhysXSDK();
	PxScene* pxScene = g_PxPhysXSystem->GetPhysXScene();
	PxMaterial* pxMat = g_PxPhysXSystem->GetDefaultPxMaterial();

	PxVec3 offsetPx(separation / 2.f, 0, 0);
	Vec3 offset(separation / 2.f, 0.f, 0.f);
	PxTransform localTm(offsetPx);
	PxRigidDynamic* prev = nullptr;

	for (int i = 0; i < length; i++)
	{
		PxRigidDynamic* current = PxCreateDynamic(*physX, transform * localTm, geometry, *pxMat, 1.0f);

		if (prev == nullptr)
		{
			CreateJointSimpleSpherical(prev, position, current, offset * -1.f);
		}
		else
		{
			CreateJointSimpleSpherical(prev, offset, current, offset * -1.f);
		}

		pxScene->addActor(*current);
		prev = current;
		localTm.p.x += separation;
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void PhysXSystem::CreateSimpleFixedChain(const Vec3& position, int length, const PxGeometry& geometry, float separation)
{
	PxTransform transform(PhysXSystem::VecToPxVector(position));
	PxPhysics* physX = g_PxPhysXSystem->GetPhysXSDK();
	PxScene* pxScene = g_PxPhysXSystem->GetPhysXScene();
	PxMaterial* pxMat = g_PxPhysXSystem->GetDefaultPxMaterial();

	PxVec3 offsetPx(separation / 2.f, 0, 0);
	Vec3 offset(separation / 2.f, 0.f, 0.f);
	PxTransform localTm(offsetPx);
	PxRigidDynamic* prev = nullptr;

	for (int i = 0; i < length; i++)
	{
		PxRigidDynamic* current = PxCreateDynamic(*physX, transform * localTm, geometry, *pxMat, 1.0f);

		if (prev == nullptr)
		{
			CreateJointSimpleFixed(prev, position, current, offset * -1.f);
		}
		else
		{
			CreateJointSimpleFixed(prev, offset, current, offset * -1.f);
		}

		pxScene->addActor(*current);
		prev = current;
		localTm.p.x += separation;
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void PhysXSystem::CreateLimitedSphericalChain(const Vec3& position, int length, const PxGeometry& geometry, float separation, float yConeAngleLimit, float zConeAngleLimit, float coneContactDistance /*= -1.f*/)
{
	PxTransform transform(PhysXSystem::VecToPxVector(position));
	PxPhysics* physX = g_PxPhysXSystem->GetPhysXSDK();
	PxScene* pxScene = g_PxPhysXSystem->GetPhysXScene();
	PxMaterial* pxMat = g_PxPhysXSystem->GetDefaultPxMaterial();

	PxVec3 offsetPx(separation / 2.f, 0, 0);
	Vec3 offset(separation / 2.f, 0.f, 0.f);
	PxTransform localTm(offsetPx);
	PxRigidDynamic* prev = nullptr;

	for (int i = 0; i < length; i++)
	{
		PxRigidDynamic* current = PxCreateDynamic(*physX, transform * localTm, geometry, *pxMat, 1.0f);

		if (prev == nullptr)
		{
			CreateJointLimitedSpherical(prev, position, current, offset * -1.f, yConeAngleLimit, zConeAngleLimit, coneContactDistance);
		}
		else
		{
			CreateJointLimitedSpherical(prev, offset, current, offset * -1.f, yConeAngleLimit, zConeAngleLimit, coneContactDistance);
		}

		pxScene->addActor(*current);
		prev = current;
		localTm.p.x += separation;
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void PhysXSystem::CreateBreakableFixedChain(const Vec3& position, int length, const PxGeometry& geometry, float separation, float breakForce /*= FLT_MAX*/, float breakTorque /*= FLT_MAX*/)
{
	PxTransform transform(PhysXSystem::VecToPxVector(position));
	PxPhysics* physX = g_PxPhysXSystem->GetPhysXSDK();
	PxScene* pxScene = g_PxPhysXSystem->GetPhysXScene();
	PxMaterial* pxMat = g_PxPhysXSystem->GetDefaultPxMaterial();

	PxVec3 offsetPx(separation / 2.f, 0, 0);
	Vec3 offset(separation / 2.f, 0.f, 0.f);
	PxTransform localTm(offsetPx);
	PxRigidDynamic* prev = nullptr;

	for (int i = 0; i < length; i++)
	{
		PxRigidDynamic* current = PxCreateDynamic(*physX, transform * localTm, geometry, *pxMat, 1.0f);

		if (prev == nullptr)
		{
			CreateJointBreakableFixed(prev, position, current, offset * -1.f, breakForce, breakTorque);
		}
		else
		{
			CreateJointBreakableFixed(prev, offset, current, offset * -1.f, breakForce, breakTorque);
		}

		pxScene->addActor(*current);
		prev = current;
		localTm.p.x += separation;
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void PhysXSystem::CreateDampedD6Chain(const Vec3& position, int length, const PxGeometry& geometry, float separation, float driveStiffness, float driveDamping, float driveForceLimit /*= FLT_MAX*/, bool isDriveAcceleration /*= false*/)
{
	PxTransform transform(PhysXSystem::VecToPxVector(position));
	PxPhysics* physX = g_PxPhysXSystem->GetPhysXSDK();
	PxScene* pxScene = g_PxPhysXSystem->GetPhysXScene();
	PxMaterial* pxMat = g_PxPhysXSystem->GetDefaultPxMaterial();

	PxVec3 offsetPx(separation / 2.f, 0, 0);
	Vec3 offset(separation / 2.f, 0.f, 0.f);
	PxTransform localTm(offsetPx);
	PxRigidDynamic* prev = nullptr;

	for (int i = 0; i < length; i++)
	{
		PxRigidDynamic* current = PxCreateDynamic(*physX, transform * localTm, geometry, *pxMat, 1.0f);

		if (prev == nullptr)
		{
			CreateJointDampedD6(prev, position, current, offset * -1.f, driveStiffness, driveDamping, driveForceLimit, isDriveAcceleration);
		}
		else
		{
			CreateJointDampedD6(prev, offset, current, offset * -1.f, driveStiffness, driveDamping, driveForceLimit, isDriveAcceleration);
		}

		pxScene->addActor(*current);
		prev = current;
		localTm.p.x += separation;
	}
}

//------------------------------------------------------------------------------------------------------------------------------
PxConvexMesh* PhysXSystem::CreateConvexMesh(const PxVec3* verts, const PxU32 numVerts, PxPhysics& physics, PxCooking& cooking)
{
	// Create descriptor for convex mesh
	PxConvexMeshDesc convexDesc;
	convexDesc.points.count = numVerts;
	convexDesc.points.stride = sizeof(PxVec3);
	convexDesc.points.data = verts;
	convexDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;

	PxConvexMesh* convexMesh = NULL;
	PxDefaultMemoryOutputStream buf;
	if (cooking.cookConvexMesh(convexDesc, buf))
	{
		PxDefaultMemoryInputData id(buf.getData(), buf.getSize());
		convexMesh = physics.createConvexMesh(id);
	}

	return convexMesh;
}

//------------------------------------------------------------------------------------------------------------------------------
PxConvexMesh* PhysXSystem::CreateWedgeConvexMesh(const PxVec3& halfExtents, PxPhysics& physics, PxCooking& cooking)
{
	PxVec3 verts[6] =
	{
		PxVec3(-halfExtents.x, -halfExtents.y, -halfExtents.z),
		PxVec3(-halfExtents.x, -halfExtents.y, +halfExtents.z),
		PxVec3(-halfExtents.x, +halfExtents.y, -halfExtents.z),
		PxVec3(+halfExtents.x, -halfExtents.y, -halfExtents.z),
		PxVec3(+halfExtents.x, -halfExtents.y, +halfExtents.z),
		PxVec3(+halfExtents.x, +halfExtents.y, -halfExtents.z)
	};
	PxU32 numVerts = 6;

	return CreateConvexMesh(verts, numVerts, physics, cooking);
}

//------------------------------------------------------------------------------------------------------------------------------
PxConvexMesh* PhysXSystem::CreateCuboidConvexMesh(const PxVec3& halfExtents, PxPhysics& physics, PxCooking& cooking)
{
	PxVec3 verts[8] =
	{
		PxVec3(-halfExtents.x, -halfExtents.y, -halfExtents.z),
		PxVec3(-halfExtents.x, -halfExtents.y, +halfExtents.z),
		PxVec3(-halfExtents.x, +halfExtents.y, -halfExtents.z),
		PxVec3(-halfExtents.x, +halfExtents.y, +halfExtents.z),
		PxVec3(+halfExtents.x, -halfExtents.y, -halfExtents.z),
		PxVec3(+halfExtents.x, -halfExtents.y, +halfExtents.z),
		PxVec3(+halfExtents.x, +halfExtents.y, -halfExtents.z),
		PxVec3(+halfExtents.x, +halfExtents.y, +halfExtents.z)
	};
	PxU32 numVerts = 8;

	return CreateConvexMesh(verts, numVerts, physics, cooking);
}

//------------------------------------------------------------------------------------------------------------------------------
PxRigidStatic* PhysXSystem::AddStaticObstacle
(const PxTransform& transform, const PxU32 numShapes, PxTransform* shapeTransforms, PxGeometry** shapeGeometries, PxMaterial** shapeMaterials)
{
	PxFilterData simFilterData;
	simFilterData.word0 = COLLISION_FLAG_GROUND;
	simFilterData.word1 = COLLISION_FLAG_GROUND_AGAINST;
	PxFilterData qryFilterData;
	setupDrivableSurface(qryFilterData);

	PxRigidStatic* actor = m_PhysX->createRigidStatic(transform);
	for (PxU32 i = 0; i < numShapes; i++)
	{
		PxShape* shape = PxRigidActorExt::createExclusiveShape(*actor, *shapeGeometries[i], *shapeMaterials[i]);
		shape->setLocalPose(shapeTransforms[i]);
		shape->setSimulationFilterData(simFilterData);
		shape->setQueryFilterData(qryFilterData);
	}
	m_PxScene->addActor(*actor);
	return actor;
}

//------------------------------------------------------------------------------------------------------------------------------
PxRigidDynamic* PhysXSystem::AddDynamicObstacle
(const PxTransform& transform, const PxF32 mass, const PxU32 numShapes, PxTransform* shapeTransforms, PxGeometry** shapeGeometries, PxMaterial** shapeMaterials)
{
	PxFilterData simFilterData;
	simFilterData.word0 = COLLISION_FLAG_OBSTACLE;
	simFilterData.word1 = COLLISION_FLAG_OBSTACLE_AGAINST;
	PxFilterData qryFilterData;
	setupNonDrivableSurface(qryFilterData);

	PxRigidDynamic* actor = m_PhysX->createRigidDynamic(transform);
	for (PxU32 i = 0; i < numShapes; i++)
	{
		PxShape* shape = PxRigidActorExt::createExclusiveShape(*actor, *shapeGeometries[i], *shapeMaterials[i]);
		shape->setLocalPose(shapeTransforms[i]);
		shape->setSimulationFilterData(simFilterData);
		shape->setQueryFilterData(qryFilterData);
	}

	PxRigidBodyExt::setMassAndUpdateInertia(*actor, mass);
	m_PxScene->addActor(*actor);
	return actor;
}

//------------------------------------------------------------------------------------------------------------------------------
physx::PxMaterial* PhysXSystem::GetDefaultPxMaterial() const
{
	return m_PxDefaultMaterial;
}

//------------------------------------------------------------------------------------------------------------------------------
STATIC Vec3 PhysXSystem::PxVectorToVec(const PxVec3& pxVector)
{
	Vec3 vector;
	vector.x = pxVector.x;
	vector.y = pxVector.y;
	vector.z = pxVector.z;
	return vector;
}

//------------------------------------------------------------------------------------------------------------------------------
STATIC Vec4 PhysXSystem::PxVectorToVec(const PxVec4& pxVector) 
{
	Vec4 vector;
	vector.x = pxVector.x;
	vector.y = pxVector.y;
	vector.z = pxVector.z;
	vector.w = pxVector.w;
	return vector;
}

//------------------------------------------------------------------------------------------------------------------------------
STATIC physx::PxVec3 PhysXSystem::VecToPxVector(const Vec3& vector)
{
	PxVec3 pxVector(vector.x, vector.y, vector.z);
	return pxVector;
}

//------------------------------------------------------------------------------------------------------------------------------
STATIC physx::PxVec4 PhysXSystem::VecToPxVector(const Vec4& vector)
{
	PxVec4 pxVector(vector.x, vector.y, vector.z, vector.w);
	return pxVector;
}

//------------------------------------------------------------------------------------------------------------------------------
STATIC Vec3 PhysXSystem::QuaternionToEulerAngles(const PxQuat& quat) 
{
	Vec3 eulerAngles;

	// roll (x-axis rotation)
	float sinr_cosp = 2.0f * (quat.w * quat.x + quat.y * quat.z);
	float cosr_cosp = 1.0f - 2.0f * (quat.x * quat.x + quat.y * quat.y);
	eulerAngles.x = atan2f(sinr_cosp, cosr_cosp);

	// pitch (y-axis rotation)
	float sinp = +2.0f * (quat.w * quat.y - quat.z * quat.x);
	if (fabs(sinp) >= 1)
		eulerAngles.y = copysign(PI / 2.f, sinp); // use 90 degrees if out of range
	else
		eulerAngles.y = asinf(sinp);

	// yaw (z-axis rotation)
	float siny_cosp = 2.0f * (quat.w * quat.z + quat.x * quat.y);
	float cosy_cosp = 1.0f - 2.0f * (quat.y * quat.y + quat.z * quat.z);
	eulerAngles.z = atan2f(siny_cosp, cosy_cosp);

	return eulerAngles;
}

//------------------------------------------------------------------------------------------------------------------------------
STATIC PxQuat PhysXSystem::MakeQuaternionFromMatrix(const Matrix44& matrix)
{
	//Quaternion
	float w = sqrt(1.0f + matrix.m_values[Matrix44::Ix] + matrix.m_values[Matrix44::Jy] + matrix.m_values[Matrix44::Kz]) / 2.0f;

	if (w == 0)
	{
		w = 1.f;
	}

	float one_over_w4 = 1.f / (4.0f * w);
	float x = (matrix.m_values[Matrix44::Ky] - matrix.m_values[Matrix44::Jz]) * one_over_w4;
	float y = (matrix.m_values[Matrix44::Iz] - matrix.m_values[Matrix44::Kx]) * one_over_w4;
	float z = (matrix.m_values[Matrix44::Jx] - matrix.m_values[Matrix44::Iy]) * one_over_w4;

	PxQuat quaternion = PxQuat(x, y, z, w);
	return quaternion;
}

//------------------------------------------------------------------------------------------------------------------------------
void PhysXSystem::ShutDown()
{
	if (m_vehicleKitEnabled)
	{
		PxCloseVehicleSDK();
	}

	//Handle all shutdown code here for Nvidia PhysX
	PX_RELEASE(m_PxScene);
	PX_RELEASE(m_PxDispatcher);
	PX_RELEASE(m_PhysX);
	PX_RELEASE(m_PxCooking);
	if (m_Pvd)
	{
		PxPvdTransport* transport = m_Pvd->getTransport();
		m_Pvd->release();	m_Pvd = NULL;
		PX_RELEASE(transport);
	}
	PX_RELEASE(m_PxFoundation);
}

