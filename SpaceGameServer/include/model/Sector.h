#ifndef _SECTOR_H_
#define _SECTOR_H_

#include "OgreVector3.h"

#include "model/StaticObject.h"
#include "model/PlanetObject.h"
#include "model/Shot.h"
#include "model/DynamicObject.h"
#include "model/Ship.h"

#include "RakNetTypes.h"
#include "btBulletDynamicsCommon.h"
#include "SpaceGameTypes.h"
#include "manager/InputHistoryManager.h"

#include <string>
#include <list>

class BulletDebugDraw;

class SectorSettings;
class ShipSettings;
class InputState;

namespace Ogre
{
	class SceneManager;
}

class Sector
{
public:
	static const float epsilon;

	Sector(Ogre::SceneManager* _sceneManager, float _sectorUpdateRate, unsigned int _maxSectorTickRewindAmount);
	~Sector();

	//Init sector and static objects
	void instantiateObjects(const std::string& _sectorName);

	//Getters
	std::vector<StaticObject*>& getStaticObjects() { return mStaticObjects;}
	std::vector<DynamicObject*>& getDynamicObjects() { return mDynamicObjects;}
	std::vector<Shot*>& getShots() { return mShots;}
	std::vector<PlanetObject*>& getPlanetObjects() { return mPlanetObjects;}
	
	//Add dynamic objects and instantiate them
	void addShotObject(const ShotSettings& _shotSettings);
	void instantiateClientShip(const RakNet::RakNetGUID& _id, Ship& _ship, const std::string& _shipId, const Ogre::Quaternion& _orientation, const Ogre::Vector3& _position, UniqueId& _shipUniqueId, SectorTick& _sectorTick);

	//Set static objects visible
	void setStaticObjectsVisible(bool _value);

	//Update function
	void updateSector();

	//Collision physic related
	btDispatcher* getDynamicWorldDispatcher() const { return mDynamicWorld->getDispatcher(); }

	//Debug view utils
	void switchDisplayDebug();
	void switchDisplay();

	//Unique id generator
	UniqueId getNextUniqueId() const { return sUniqueId++; }

	//Serialize
	void serialize(RakNet::BitStream& _bitStream) const;

	//Add input for a client
	void addInput(const RakNet::RakNetGUID& _id, SectorTick _tick, const InputState& _clientInput);

protected:
	static UniqueId sUniqueId;

	Ogre::SceneManager* mSceneManager = nullptr;

	//Sector objects
	std::vector<StaticObject*> mStaticObjects;
	std::vector<DynamicObject*> mDynamicObjects;
	std::vector<Shot*> mShots;
	std::vector<PlanetObject*> mPlanetObjects;
	std::vector<SectorObject*> mGateObjects;
	std::map<RakNet::RakNetGUID, Ship*> mShips;

	//Users in sector
	std::set<RakNet::RakNetGUID> mUsersIds;

	//The physic world
	btDiscreteDynamicsWorld* mDynamicWorld = nullptr;
	//Physic simulation related
	btDbvtBroadphase* mBroadphase = nullptr;
    btDefaultCollisionConfiguration* mCollisionConfiguration = nullptr;
    btCollisionDispatcher* mDispatcher = nullptr;
	btSequentialImpulseConstraintSolver* mConstraintSolver = nullptr;

	//Update ships systems
	void updateShipsSystems(float _deltaTime, const ClientsInputMap& _clientsInputMap);
	//Simulate from oldest acceptable input to the last one
	//Return true if a rewind was performed
	bool simulateWorldForClientsHistory();

	//Game tick
	SectorTick mSectorTick = 0;

	//Update rate
	float mSectorUpdateRate = 0.f;

	//Clients input history of the sector
	InputHistoryManager mClientsInput;

	//Set all sector's objects to the state they were at tick
	void setSectorState(SectorTick _tick);
	//Save all entities states (not the sector himself)
	void saveSectorState();

	//Debug view utils
	BulletDebugDraw* mBulletDebugDraw = nullptr;
	bool mDisplayDebug = false;
	bool mDoDisplayWorld = true;
};

#endif //_SECTOR_H_