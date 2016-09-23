#include "model/Sector.h"

#include "model/StaticObject.h"
#include "model/GameSettings.h"
#include "model/InputState.h"
#include "model/ObjectPart.h"
#include "model/HardPoint.h"

#include "controller/SectorController.h"

#include "utils/StringUtils.h"
#include "utils/OgreUtils.h"
#include "utils/OgreBulletConvert.h"
#include "utils/BulletDebugDraw.h"
#include "manager/LoggerManager.h"

#include "network/NetworkService.h"

#include "OgreSceneNode.h"

#include "RakNetTypes.h"
#include "BitStream.h"

namespace
{
	const std::string LOG_CLASS_TAG = "Sector";
}

UniqueId Sector::sUniqueId = 1;

//Epsilon
//Ogre::Real epsilon = std::numeric_limits<Ogre::Real>::epsilon();
const float Sector::epsilon = 0.001f;

//Custom overlapping filtering callback
//Allow parts of the ship to collide with parts of other ship but don't test collision with other part of its own ship
/*struct ShipPartsFilterCallback : public btOverlapFilterCallback
{
	// return true when pairs need collision
	virtual bool needBroadphaseCollision(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1) const
	{
		//Basic filter and mask filtering
		bool collides = (proxy0->m_collisionFilterGroup & proxy1->m_collisionFilterMask) != 0;
		collides &= proxy1->m_collisionFilterGroup & proxy0->m_collisionFilterMask;

		if(collides)
		{
			//Additional custom filtering
			//btCollisionObject* collisionObject0 = static_cast<btCollisionObject*>(proxy0->m_clientObject);
			//StaticObject* staticObject0 = static_cast<StaticObject*>(collisionObject0->getUserPointer());
			//btCollisionObject* collisionObject1 = static_cast<btCollisionObject*>(proxy1->m_clientObject);
			//StaticObject* staticObject1 = static_cast<StaticObject*>(collisionObject1->getUserPointer());

			//Same id mean part of the same object
			//collides &= staticObject0->getUniqueId() != staticObject1->getUniqueId();
		}

		return collides;
	}
};*/

void MyTickCallback(btDynamicsWorld* _world, btScalar _timeStep)
{
	Sector* sector = (Sector*)_world->getWorldUserInfo();
	
	//Collisions damages 
	int numManifolds = sector->getDynamicWorldDispatcher()->getNumManifolds();
	for (int i = 0; i < numManifolds; i++)
	{
		btPersistentManifold* contactManifold = sector->getDynamicWorldDispatcher()->getManifoldByIndexInternal(i);
		
		if(contactManifold->getNumContacts() > 0)
		{
			const btRigidBody* object0 = static_cast<const btRigidBody*>(contactManifold->getBody0());
			const btRigidBody* object1 = static_cast<const btRigidBody*>(contactManifold->getBody1());
			
			//We deal with compound shapes only
			const btCompoundShape* compound0 = static_cast<const btCompoundShape*>(contactManifold->getBody0()->getCollisionShape());
			const btCompoundShape* compound1 = static_cast<const btCompoundShape*>(contactManifold->getBody1()->getCollisionShape());
			
			if(compound0 && compound1 && compound0->getShapeType() == COMPOUND_SHAPE_PROXYTYPE && compound1->getShapeType() == COMPOUND_SHAPE_PROXYTYPE && contactManifold->getNumContacts() > 0)
			{
				const btCollisionShape* childShape0 = compound0->getChildShape(object0->getUserIndex());
				const btCollisionShape* childShape1 = compound1->getChildShape(object1->getUserIndex());

				ObjectPart* objectPart0 = (ObjectPart*)childShape0->getUserPointer();
				ObjectPart* objectPart1 = (ObjectPart*)childShape1->getUserPointer();

				for(int i = 0; i < contactManifold->getNumContacts(); ++i)
				{
					float damage = contactManifold->getContactPoint(i).getAppliedImpulse();
					if(damage > 1.f)
					{
						objectPart0->mHitPoints -= (int)contactManifold->getContactPoint(i).getAppliedImpulse();
						objectPart1->mHitPoints -= (int)contactManifold->getContactPoint(i).getAppliedImpulse();
					}
				}
			}
		}
	}
}

//Use mRigidBody->setCollisionFlags(mRigidBody->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK); to trigger this callback on a rigidBody
bool MyContactCallback(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1)
{
	btCompoundShape* compound0 = (btCompoundShape*)colObj0Wrap->getCollisionObject()->getCollisionShape();
	btCompoundShape* compound1 = (btCompoundShape*)colObj1Wrap->getCollisionObject()->getCollisionShape();

	StaticObject* staticObject0 = (StaticObject*)compound0->getUserPointer();
	StaticObject* staticObject1 = (StaticObject*)compound1->getUserPointer();

	staticObject0->getRigidBody()->setUserIndex(index0);
	staticObject1->getRigidBody()->setUserIndex(index1);
	
	return true;
}

Sector::Sector(Ogre::SceneManager* _sceneManager, float _sectorUpdateRate, unsigned int _maxSectorTickRewindAmount)
	: mSceneManager(_sceneManager),
	mSectorUpdateRate(_sectorUpdateRate),
	mClientsInput(_maxSectorTickRewindAmount)
{
	//Create dynamic world
	mBroadphase = new btDbvtBroadphase();
    mCollisionConfiguration = new btDefaultCollisionConfiguration();
    mDispatcher = new btCollisionDispatcher(mCollisionConfiguration);
	mConstraintSolver = new btSequentialImpulseConstraintSolver();
 
	mDynamicWorld = new btDiscreteDynamicsWorld(mDispatcher, mBroadphase, mConstraintSolver, mCollisionConfiguration);
	mDynamicWorld->setGravity(btVector3(0,0,0));
	mDynamicWorld->setInternalTickCallback(MyTickCallback, this);

	/*ShipPartsFilterCallback* shipPartsFilterCallback = new ShipPartsFilterCallback();
	mDynamicWorld->getPairCache()->setOverlapFilterCallback(shipPartsFilterCallback);*/

	gContactAddedCallback = &MyContactCallback;

	//Init bullet debugger
	mBulletDebugDraw = new BulletDebugDraw(mSceneManager, mDynamicWorld);
	mBulletDebugDraw->setDebugMode(btIDebugDraw::DBG_DrawContactPoints | btIDebugDraw::DBG_DrawWireframe | btIDebugDraw::DBG_DrawAabb);
	mDisplayDebug = false;
}

void Sector::instantiateObjects(const std::string& _sectorName)
{
	LoggerManager::getInstance().logI(LOG_CLASS_TAG, "instantiateObjects", "Instantiating objects", false);

	const SectorSettings* sectorSettings = GameSettings::getInstance().getSector(_sectorName);

	//Fill in objects
	if(sectorSettings)
	{
		//Static objects
		for(int i = 0; i < sectorSettings->mStaticObjects.size(); ++i)
		{
			mStaticObjects.push_back(new StaticObject(&sectorSettings->mStaticObjects[i], mSceneManager, mDynamicWorld));
			mStaticObjects.back()->instantiateObject();
		}

		//Planet objects
		for(int i = 0; i < sectorSettings->mPlanetObjects.size(); ++i)
		{
			mPlanetObjects.push_back(new PlanetObject(&sectorSettings->mPlanetObjects[i], mSceneManager));
			mPlanetObjects.back()->instantiateObject();
		}

		//Gate objects
		for(int i = 0; i < sectorSettings->mGateObjects.size(); ++i)
		{
			mGateObjects.push_back(new SectorObject(&sectorSettings->mGateObjects[i], mSceneManager));
			mGateObjects.back()->instantiateObject();
		}
	}
	else
	{
		LoggerManager::getInstance().logE(LOG_CLASS_TAG, "instantiateObjects", "Sector settings not found for sector : " + _sectorName);
		assert(false);
	}
}

void Sector::addShotObject(const ShotSettings& _shotSettings)
{
	Shot* newShot = new Shot(&_shotSettings, mSceneManager);
	newShot->instantiateObject();
	mShots.push_back(newShot);
}

void Sector::instantiateClientShip(const RakNet::RakNetGUID& _id, Ship& _ship, const std::string& _shipId, const Ogre::Quaternion& _orientation, const Ogre::Vector3& _position, UniqueId& _shipUniqueId, SectorTick& _sectorTick)
{
	std::map<RakNet::RakNetGUID, Ship*>::const_iterator foundItem = mShips.find(_id);
	if(foundItem == mShips.end())
	{
		_sectorTick = mSectorTick;
		_shipUniqueId = getNextUniqueId();
		_ship.instantiateObject(mSceneManager, mDynamicWorld, _shipUniqueId);
		_ship.forceWorldTransform(btTransform(convert(_orientation), convert(_position)));
		mShips[_id] = &_ship;
		mUsersIds.insert(_id);

		LoggerManager::getInstance().logI(LOG_CLASS_TAG, "instantiateClientShip", "_sectorTick : " + StringUtils::toStr(_sectorTick) + "; _shipUniqueId : " + StringUtils::toStr(_shipUniqueId) + "; (GUID)_id : " + std::string(_id.ToString()) + "; _shipId : " + _shipId, false);

		//We want at least one neutral input
		addInput(_id, mSectorTick, InputState());
	}
	else
	{
		LoggerManager::getInstance().logE(LOG_CLASS_TAG, "instantiateClientShip", "A ship already exists for _id : " + std::string(_id.ToString()));
		assert(false);
	}
}

void Sector::setStaticObjectsVisible(bool _value)
{
	for(size_t i = 0; i < mStaticObjects.size(); ++i)
	{
		mStaticObjects[i]->setVisible(_value);
	}
}

Sector::~Sector()
{
	for(size_t i = 0; i < mStaticObjects.size(); ++i)
	{
		mStaticObjects[i]->destroy();
	}

	for(size_t i = 0; i < mDynamicObjects.size(); ++i)
	{
		mDynamicObjects[i]->destroy();
	}

	for(size_t i = 0; i < mPlanetObjects.size(); ++i)
	{
		mPlanetObjects[i]->destroy();
	}

	for(size_t i = 0; i < mShots.size(); ++i)
	{
		mShots[i]->destroy();
	}

	for(std::map<RakNet::RakNetGUID, Ship*>::iterator shipIt = mShips.begin(), shipItEnd = mShips.end(); shipIt != shipItEnd; ++shipIt)
	{
		(*shipIt).second->destroy();
	}

	mBulletDebugDraw->stop();
}

void Sector::updateSector()
{
	LoggerManager::getInstance().logI(LOG_CLASS_TAG, "updateSector", "START: mSectorTick is : " + StringUtils::toStr(mSectorTick), false);

	//resimulate from this input
	//lastSimulatedInputTick = 0 if no rewind was performed, so clients were up to date at last broadcast
	//lastSimulatedInputTick != 0 if a rewind was performed, so clients need new state at last input tick simulated (!= mSectorTick)
	bool needBroadcast = simulateWorldForClientsHistory();

	saveSectorState();

	if(needBroadcast)
	{
		LoggerManager::getInstance().logI(LOG_CLASS_TAG, "updateSector", "Broadcasting to clients mSectorTick is : " + StringUtils::toStr(mSectorTick), false);

		//Sector state and last input broadcasting
		RakNet::BitStream bitStream;
		this->serialize(bitStream);
		NetworkService::getInstance().broadcastSector(mUsersIds, bitStream);
	}

	LoggerManager::getInstance().logI(LOG_CLASS_TAG, "updateSector", "BEFORE Updating mSectorTick : " + StringUtils::toStr(mSectorTick), false);
	mSectorTick++;
	LoggerManager::getInstance().logI(LOG_CLASS_TAG, "updateSector", "AFTER Updating mSectorTick : " + StringUtils::toStr(mSectorTick), false);

	mClientsInput.update(mSectorTick);

	LoggerManager::getInstance().logI(LOG_CLASS_TAG, "updateSector", "END: mSectorTick is : " + StringUtils::toStr(mSectorTick), false);
}

bool Sector::simulateWorldForClientsHistory()
{
	ClientsInputMap clientsInputMap;
	SectorTick currentSectorTick = 0;
	//The check for oldest possible rewind is done here
	bool needRewind = mClientsInput.getOldestUnsimulatedInput(clientsInputMap, currentSectorTick);

	LoggerManager::getInstance().logI(LOG_CLASS_TAG, "simulateWorldForClientsHistory", "Oldest unsimulated is " + StringUtils::toStr(currentSectorTick), false);

	//Rewind to a state then simulate until last input
	if(needRewind)
	{
		LoggerManager::getInstance().logI(LOG_CLASS_TAG, "simulateWorldForClientsHistory", "Need rewind.", false);

		//Set sector state to currentSectorTick
		setSectorState(currentSectorTick);

		while(true)
		{
			LoggerManager::getInstance().logI(LOG_CLASS_TAG, "simulateWorldForClientsHistory", "Resimulate tick " + StringUtils::toStr(currentSectorTick), false);

			updateShipsSystems(mSectorUpdateRate, clientsInputMap);
			mDynamicWorld->stepSimulation(mSectorUpdateRate, 1, mSectorUpdateRate);

			currentSectorTick++;

			if(currentSectorTick > mSectorTick)
				break;

			mClientsInput.getLastInputForAllClients(currentSectorTick, clientsInputMap);
		}
	}
	else
	{
		LoggerManager::getInstance().logI(LOG_CLASS_TAG, "simulateWorldForClientsHistory", "Don't need rewind. Simulate current tick : " + StringUtils::toStr(mSectorTick), false);
		updateShipsSystems(mSectorUpdateRate, clientsInputMap);
		mDynamicWorld->stepSimulation(mSectorUpdateRate, 1, mSectorUpdateRate);
	}

	return needRewind;
}

/*void Sector::updateShots(float _deltaTime)
{
	Shot* shot;
	btVector3 oldPosition, newPosition;
	std::vector<Shot*>& shotsList = mCurrentSector->getShots();
	for(size_t i = 0; i < shotsList.size();)
	{
		shot = shotsList[i];

		//Update life period
		shotsList[i]->mTimeElapsed += _timeSinceLastFrame;
		//Auto remove after life period
		if(shotsList[i]->mTimeElapsed > shotsList[i]->getLifeTime())
		{
			shotsList[i]->destroy();
			delete shotsList[i];
			shotsList.erase(shotsList.begin() + i);
			continue;
		}

		oldPosition = convert(shot->getSceneNode()->getPosition());
		newPosition = convert(shot->getSceneNode()->getPosition()) + convert(shot->mSpeed * _timeSinceLastFrame);

		btDiscreteDynamicsWorld::ClosestRayResultCallback closestRayResultCallback(oldPosition, newPosition);
		mDynamicWorld->rayTest(oldPosition, newPosition, closestRayResultCallback);
		
		if(closestRayResultCallback.hasHit())
		{
			StaticObject* shotObject = (StaticObject*)closestRayResultCallback.m_collisionObject->getCollisionShape()->getUserPointer();
			//mLastShotTarget = shotObject->getName();
			shotsList[i]->destroy();
			delete shotsList[i];
			shotsList.erase(shotsList.begin() + i);
		}
		else
		{
			shot->getSceneNode()->setPosition(convert(newPosition));
			++i;
		}
	}
}*/

void Sector::updateShipsSystems(float _deltaTime, const ClientsInputMap& _clientsInputMap)
{
	LoggerManager::getInstance().logI(LOG_CLASS_TAG, "updateShipsSystems", "", false);

	for(std::map<RakNet::RakNetGUID, Ship*>::const_iterator shipIt = mShips.begin(), shipItEnd = mShips.end(); shipIt != shipItEnd; ++shipIt)
	{
		RakNet::RakNetGUID clientId = (*shipIt).first;
		Ship* clientShip = (*shipIt).second;
		InputState& clientInput = InputState();

		//If we find input for the client we use it, else we use default
		//With no update from client, input is unchanged, meaning that a key pressed is still pressed until client says i not anymore
		ClientsInputMap::const_iterator foundClientInput = _clientsInputMap.find(clientId);
		if(foundClientInput != _clientsInputMap.end())
		{
			clientInput = (*foundClientInput).second;
		}
		else
		{
			LoggerManager::getInstance().logE(LOG_CLASS_TAG, "updateShipsSystems", "No input found for clientId : " + std::string(clientId.ToString()));
			assert(false);
		}

		///////////////////
		//Handling thrust//
		///////////////////
		if(clientInput.mZKeyPressed)
			clientShip->getEngine().mWantedThrust += clientShip->getEngine().getThrustSensitivity() * _deltaTime;
		
		if(clientInput.mSKeyPressed)
			clientShip->getEngine().mWantedThrust -= clientShip->getEngine().getThrustSensitivity() * _deltaTime;
	
		if(clientInput.mAKeyPressed)
			clientShip->getEngine().mWantedThrust = clientShip->getEngine().getThrustMaxValue();
		
		if(clientInput.mQKeyPressed)
			clientShip->getEngine().mWantedThrust = 0;

		//Checking thrust bounds values
		if(clientShip->getEngine().mWantedThrust > clientShip->getEngine().getThrustMaxValue())
			clientShip->getEngine().mWantedThrust = clientShip->getEngine().getThrustMaxValue();
		if(clientShip->getEngine().mWantedThrust < 0)
			clientShip->getEngine().mWantedThrust = 0;

		//Adding or removing thrust
		float deltaThrust = clientShip->getEngine().mWantedThrust - clientShip->getEngine().mRealThrust;
		float thrustToAdd = 0.f;
		if(std::fabs(deltaThrust) < clientShip->getEngine().getReactivity())
		{
			thrustToAdd = (std::fabs(deltaThrust) / 2.f);
		}
		else
		{
			thrustToAdd = clientShip->getEngine().getReactivity();
		}

		if(deltaThrust > epsilon)
		{
			clientShip->getEngine().mRealThrust += thrustToAdd * _deltaTime;
		}
		else
		{
			clientShip->getEngine().mRealThrust = clientShip->getEngine().mWantedThrust;
		}
	
		///////////////////////////////////
		//Applying mouse movement on ship//
		///////////////////////////////////
		//Yaw
		if(!clientInput.mWKeyPressed && !clientInput.mXKeyPressed)
			clientShip->mCurrentYawForce = 0.f;
		else if(clientInput.mWKeyPressed)
			clientShip->mCurrentYawForce = -clientShip->getMaxYawRate() * clientShip->getDirectional().getTurnRateMultiplier();
		else
			clientShip->mCurrentYawForce = clientShip->getMaxYawRate() * clientShip->getDirectional().getTurnRateMultiplier();

		//Pitch
		if(clientInput.mMouseYAbs != 0.f)
		{
			//Mouse control
			clientShip->mCurrentPitchForce = clientInput.mMouseYAbs * clientShip->getMaxPitchRate() * clientShip->getDirectional().getTurnRateMultiplier();
		}
		else
		{
			//Keyboard control
			if(!clientInput.mUpKeyPressed && !clientInput.mDownKeyPressed)
				clientShip->mCurrentPitchForce = 0.f;
			else if(clientInput.mUpKeyPressed)
				clientShip->mCurrentPitchForce = -clientShip->getMaxPitchRate() * clientShip->getDirectional().getTurnRateMultiplier();
			else
				clientShip->mCurrentPitchForce = clientShip->getMaxPitchRate() * clientShip->getDirectional().getTurnRateMultiplier();
		}

		//Roll
		if(clientInput.mMouseXAbs != 0.f)
		{
			//Mouse control
			clientShip->mCurrentRollForce = clientInput.mMouseXAbs * clientShip->getMaxRollRate() * clientShip->getDirectional().getTurnRateMultiplier();
		}
		else
		{
			//Keyboard control
			if(!clientInput.mLeftKeyPressed && !clientInput.mRightKeyPressed)
				clientShip->mCurrentRollForce = 0.f;
			else if(clientInput.mLeftKeyPressed)
				clientShip->mCurrentRollForce = -clientShip->getMaxRollRate() * clientShip->getDirectional().getTurnRateMultiplier();
			else
				clientShip->mCurrentRollForce = clientShip->getMaxRollRate() * clientShip->getDirectional().getTurnRateMultiplier();
		}
	
		//Add shot
		if(clientInput.mFirePressed)
		{
			btAlignedObjectArray<HardPoint*> hardPoints = clientShip->getHardPoints();
			for(int i = 0; i < hardPoints.size(); ++i)
			{
				const WeaponSettings* weaponSettings = &hardPoints[i]->getWeaponSettings();
				if(hardPoints[i]->isUsed() && hardPoints[i]->mElapsedTime > weaponSettings->mFireRate)
				{
					hardPoints[i]->mElapsedTime = 0.f;
					ShotSettings shotSettings = hardPoints[i]->getShotSettings(); //Create a copy to be able to modify it
					shotSettings.mInitialOrientation = clientShip->getSceneNode()->getOrientation();
					shotSettings.mInitialPosition = clientShip->getRelativePosition(convert(hardPoints[i]->getWeaponSettings().mNoslePosition + hardPoints[i]->getPosition()));
					addShotObject(shotSettings);
				}
			}
		}

		clientShip->updateHardPoints(_deltaTime);

		/////////////////////////////////////////
		//Applying ship engine power and thrust//
		/////////////////////////////////////////
		clientShip->mEnginePotentialForce = -clientShip->getEngine().mRealThrust;
		clientShip->mEnginePotentialForce *= clientShip->getEngine().getPower();

		clientShip->updateForces();
	}
}

void Sector::switchDisplayDebug()
{
	mDisplayDebug = !mDisplayDebug;
	if(mDisplayDebug)
		mBulletDebugDraw->start();
	else
		mBulletDebugDraw->stop();
}

void Sector::switchDisplay()
{
	mDoDisplayWorld = !mDoDisplayWorld;
	setStaticObjectsVisible(mDoDisplayWorld);
}

void Sector::serialize(RakNet::BitStream& _bitStream) const
{
	_bitStream.Write(mSectorTick);

	//serialize last input for each client
	ClientsInputMap lastClientsInputMap;
	//mClientsInput.getLastInputForAllClients(mSectorTick, lastClientsInputMap);
	//_bitStream.Write(lastClientsInputMap);

	//Serialize ships
	_bitStream.Write(mShips.size());
	const std::map<RakNet::RakNetGUID, Ship*>::const_iterator shipsItEnd = mShips.end();
	for(std::map<RakNet::RakNetGUID, Ship*>::const_iterator shipsIt = mShips.begin(); shipsIt != shipsItEnd; ++shipsIt)
	{
		_bitStream.Write((*shipsIt).first);
		(*shipsIt).second->serialize(_bitStream);
	}

	//TODO dynamic objects
	//TODO shots
}

void Sector::setSectorState(SectorTick _tick)
{
	LoggerManager::getInstance().logI(LOG_CLASS_TAG, "setSectorState", "_tick is : " + StringUtils::toStr(_tick), false);

	for(std::map<RakNet::RakNetGUID, Ship*>::iterator shipIt = mShips.begin(), shipItEnd = mShips.end(); shipIt != shipItEnd; ++shipIt)
	{
		(*shipIt).second->setState(_tick);
	}

	//TODO other kind of entities
}

void Sector::saveSectorState()
{
	LoggerManager::getInstance().logI(LOG_CLASS_TAG, "saveSectorState", "mSectorTick is : " + StringUtils::toStr(mSectorTick), false);

	for(std::map<RakNet::RakNetGUID, Ship*>::iterator shipIt = mShips.begin(), shipItEnd = mShips.end(); shipIt != shipItEnd; ++shipIt)
	{
		(*shipIt).second->saveState(mSectorTick);
	}

	//TODO other kind of entities
}

void Sector::addInput(const RakNet::RakNetGUID& _id, SectorTick _tick, const InputState& _clientInput)
{
	mClientsInput.addInput(_id, _tick, _clientInput);
}