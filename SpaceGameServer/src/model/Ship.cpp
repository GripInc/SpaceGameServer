#include "model/Ship.h"

#include "model/GameSettings.h"
#include "model/ObjectPart.h"
#include "model/HardPoint.h"

#include "OgreSceneNode.h"

#include "BitStream.h"
#include "utils/OgreBulletConvert.h"
#include "utils/StringUtils.h"

#include "BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h"
#include "BulletDynamics/Dynamics/btRigidBody.h"
#include "BulletCollision/CollisionShapes/btCompoundShape.h"

#include "view/PlayerCamera.h"

void Ship::initShip(const ShipSettings* _shipSettings)
{
	mObjectSettings = _shipSettings;

	mMaxYawRate = _shipSettings->mMaxYawRate;
	mMaxPitchRate = _shipSettings->mMaxPitchRate;
	mMaxRollRate = _shipSettings->mMaxRollRate;
	mCargoSpace = _shipSettings->mCargoSpace;

	mHardPoints.resize(_shipSettings->mHardPoints.size());
	for(int i = 0; i < _shipSettings->mHardPoints.size(); ++i)
	{
		mHardPoints[_shipSettings->mHardPoints[i].mIndex] = new HardPoint(_shipSettings->mHardPoints[i].mIndex, _shipSettings->mHardPoints[i].mPosition, _shipSettings->mHardPoints[i].mRoll);
	}
}

void Ship::instantiateObject(Ogre::SceneManager* _sceneManager, btDiscreteDynamicsWorld* _dynamicWorld, UniqueId _uniqueId)
{
	mSceneManager = _sceneManager;
	mDynamicWorld = _dynamicWorld;

	DynamicObject(static_cast<const DynamicObjectSettings*>(mObjectSettings), mSceneManager, mDynamicWorld, _uniqueId);

	//We explicitely want the graphics only
	SectorObject::instantiateObject();

	for(int i = 0; i < mHardPoints.size(); ++i)
	{
		if(mHardPoints[i]->isUsed())
		{
			//Find the weapon attached to hardpoint
			const WeaponSettings& weaponSettings = mHardPoints[i]->getWeaponSettings();

			addSubSceneNode(convert(btQuaternion(btVector3(0.f, 0.f, 1.f), mHardPoints[i]->getRoll())), convert(weaponSettings.mHardPoint.mPosition + mHardPoints[i]->getPosition()), Ogre::Vector3(1.f ,1.f ,1.f), weaponSettings.mMesh, "hardpoint_" + StringUtils::toStr(i) + weaponSettings.mName, mSceneManager);
		}
	}

	instantiateObjectParts();

	instantiateCollisionObject();
}

void Ship::instantiateObjectParts()
{
	DynamicObject::instantiateObjectParts();

	//Loop on used hardpoints
	for(int i = 0; i < mHardPoints.size(); ++i)
	{
		if(mHardPoints[i]->isUsed())
		{
			mObjectParts.push_back(new ObjectPart());
			ObjectPartSettings objectPartSettings;
			objectPartSettings.mName = "hardPoint_" + StringUtils::toStr(mHardPoints[i]->getIndex());
			objectPartSettings.mHitPoints = mHardPoints[i]->getWeaponSettings().mHitPoints;
			mObjectParts[mObjectParts.size() - 1]->init(objectPartSettings);

			//Get the weapon attached to hardpoint
			const WeaponSettings& weaponSettings = mHardPoints[i]->getWeaponSettings();

			//Add shapes
			const btAlignedObjectArray<CollisionShapeSettings>& collisionShapesSettings = weaponSettings.mCollisionShapes;
			for(int j = 0; j < collisionShapesSettings.size(); ++j)
			{
				btCollisionShape* collisionShape = mObjectParts[mObjectParts.size() - 1]->createCollisionShape(collisionShapesSettings[j]);
				mCollisionShapes.push_back(collisionShape);
				mCompoundShape->addChildShape(btTransform(collisionShapesSettings[j].mInitialOrientation * btQuaternion(btVector3(0.f, 0.f, 1.f), mHardPoints[i]->getRoll()), collisionShapesSettings[j].mInitialPosition + weaponSettings.mHardPoint.mPosition + mHardPoints[i]->getPosition()), collisionShape);
			}
		}
	}
}

void Ship::forceWorldTransform(const btTransform& _worldTransform)
{
	mRigidBody->setWorldTransform(_worldTransform);
}

void Ship::addEngine(const EngineSettings& _engine)
{
	mEngine.init(_engine);
}

void Ship::addDirectional(const DirectionalSettings& _directional)
{
	mDirectional.init(_directional);
}

void Ship::addWeapon(const WeaponSettings& _weapon, int _index)
{
	mHardPoints[_index]->attachWeapon(_weapon);
}

const WeaponSettings& Ship::removeWeapon(int _index)
{
	const WeaponSettings& result = mHardPoints[_index]->getWeaponSettings();
	mHardPoints[_index]->detachWeapon();

	return result;
}

void Ship::updateForces()
{
	btTransform shipTransform;
	mMyMotionState->getWorldTransform(shipTransform);

	//Compute thrust force
	float currentSpeed = mRigidBody->getLinearVelocity().length();
	float force = 0.f;
	if(currentSpeed < mEngine.getMaxSpeed())
		force = mEnginePotentialForce * std::log(mEngine.getMaxSpeed() + 1.f - currentSpeed) / std::log(mEngine.getMaxSpeed() + 1.f);

	//DEBUG
	mEngineRealForce = force;
	
	//Apply thrust
	btVector3 thrustForce(0.f, 0.f, force);
	thrustForce = rotate(thrustForce, shipTransform.getRotation());
	mRigidBody->applyCentralForce(thrustForce);

	//TODO try less instructions
	//Apply rotations
	btVector3 torque;
	torque.setValue(mCurrentPitchForce, 0.f, 0.f);
	torque = rotate(torque, shipTransform.getRotation());
	mRigidBody->applyTorque(torque);

	torque.setValue(0.f, -mCurrentYawForce, 0.f);
	torque = rotate(torque, shipTransform.getRotation());
	mRigidBody->applyTorque(torque);

	torque.setValue(0.f, 0.f, -mCurrentRollForce);
	torque = rotate(torque, shipTransform.getRotation());
	mRigidBody->applyTorque(torque);
}

const btVector3& Ship::getLinearVelocity() const
{
	return mRigidBody->getLinearVelocity();
}

void Ship::destroy()
{
	DynamicObject::destroy();
}

void Ship::updateHardPoints(float _deltaTime)
{
	for(int i = 0; i < mHardPoints.size(); ++i)
	{
		if(mHardPoints[i]->isUsed())
			mHardPoints[i]->mElapsedTime += _deltaTime;
	}
}

void Ship::saveState(SectorTick _tick)
{
	std::vector<std::pair<int, float> > hardpointsState;
	int hardpointsSize = mHardPoints.size();
	for(int i = 0; i < hardpointsSize; ++i)
	{
		if(mHardPoints[i]->isUsed())
		{
			hardpointsState.push_back(std::make_pair(mHardPoints[i]->getIndex(), mHardPoints[i]->mElapsedTime));
		}
	}

	mStateManager.saveState(_tick, ShipState(mRigidBody, mCurrentRollForce, mCurrentYawForce, mCurrentPitchForce, mEnginePotentialForce, hardpointsState));
}

void Ship::setState(SectorTick _tick)
{
	ShipState shipState;
	mStateManager.getState(_tick, shipState);

	//Rigid body
	mRigidBody->setWorldTransform(shipState.mWorldTransform);
	mRigidBody->setLinearVelocity(shipState.mLinearVelocity);
	mRigidBody->setAngularVelocity(shipState.mAngularVelocity);
	mRigidBody->applyCentralForce(shipState.mTotalForce);
	mRigidBody->applyTorque(shipState.mTotalTorque);

	//Systems
	mCurrentRollForce = shipState.mCurrentRollForce;
	mCurrentYawForce = shipState.mCurrentYawForce;
	mCurrentPitchForce = shipState.mCurrentPitchForce;

	mEnginePotentialForce = shipState.mEnginePotentialForce;

	//Hardpoints
	const std::vector<std::pair<int, float> >& harpointsState = shipState.mHarpointsState;
	int hardpointsSize = harpointsState.size();
	for(int i = 0; i < hardpointsSize; ++i)
	{
		int index = harpointsState[i].first;
		if(mHardPoints[index]->isUsed())
		{
			mHardPoints[index]->mElapsedTime = harpointsState[i].second;
		}
	}
}

void Ship::serialize(RakNet::BitStream& _bitStream) const
{
	//Unique id
	_bitStream.Write(mUniqueId);

	//State
	std::vector<std::pair<int, float> > hardpointsState;
	int hardpointsSize = mHardPoints.size();
	for(int i = 0; i < hardpointsSize; ++i)
	{
		if(mHardPoints[i]->isUsed())
		{
			hardpointsState.push_back(std::make_pair(mHardPoints[i]->getIndex(), mHardPoints[i]->mElapsedTime));
		}
	}

	ShipState shipState(mRigidBody, mCurrentRollForce, mCurrentYawForce, mCurrentPitchForce, mEnginePotentialForce, hardpointsState);
	shipState.serialize(_bitStream);
}