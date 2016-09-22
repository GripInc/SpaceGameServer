#ifndef _SECTOR_CONTROLLER_H_
#define _SECTOR_CONTROLLER_H_

#include <string>

#include "OgrePrerequisites.h"
#include "LinearMath/btScalar.h"

#include "model/Sector.h"
#include "SpaceGameTypes.h"

class SectorView;
class Ship;
class InputState;

class SectorController
{
public:
	class ClientInputBuffer
	{
	public:
		SectorTick mLastSimulatedInputTick;
		std::map<SectorTick, InputState> mInputBuffer;
	};

	SectorController() 
		: mCurrentSector(NULL),
		mSectorView(NULL)
	{}

	void createSector(const std::string& _sectorName, Ogre::SceneManager* _sceneManager, float _sectorUpdateRate, unsigned int _maxSectorTickRewindAmount);

	//Return the sector tick when the ship was created
	void instantiateClientShip(const RakNet::RakNetGUID& _id, Ship& _ship, const std::string& _shipId, const Ogre::Vector3& _position, const Ogre::Quaternion& _orientation, UniqueId& _shipUniqueId, SectorTick& _sectorTick) { return mCurrentSector->instantiateClientShip(_id, _ship, _shipId, _orientation, _position, _shipUniqueId, _sectorTick); }

	void addShotObject(const ShotSettings& _shotSettings);

	//Getters
	Sector* getCurrentSector() { return mCurrentSector; }

	//Update function
	void updateSector() { mCurrentSector->updateSector(); }

	//Add input for a client in a sector
	void addInput(const RakNet::RakNetGUID& _id, SectorTick _tick, const InputState& _clientInput);

	//DEBUG
	std::string mLastShotTarget;
	std::string mLastCollidedPart0;
	std::string mLastCollidedPart1;
	float mLastCollisionSpeed;
	void switchDisplayDebug() { mCurrentSector->switchDisplayDebug(); }
	void switchDisplay() { mCurrentSector->switchDisplay(); }

protected:
	Sector* mCurrentSector;
	SectorView* mSectorView;
};

#endif //_SECTOR_CONTROLLER_H_