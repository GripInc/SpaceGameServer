#include "controller/SectorController.h"

#include "model/InputState.h"

#include "view/SectorView.h"

#include "manager/LoggerManager.h"

namespace
{
	const std::string LOG_CLASS_TAG = "SectorController";
}

void SectorController::createSector(const std::string& _sectorName, Ogre::SceneManager* _sceneManager, float _sectorUpdateRate, unsigned int _maxSectorTickRewindAmount)
{
	LoggerManager::getInstance().logI(LOG_CLASS_TAG, "createSector", "", false);

	//Init sector
	if(mCurrentSector != NULL)
	{
		LoggerManager::getInstance().logI(LOG_CLASS_TAG, "createSector", "Deleting sector because it was not NULL", false);
		delete mCurrentSector;
	}

	mCurrentSector = new Sector(_sceneManager, _sectorUpdateRate, _maxSectorTickRewindAmount);

	// Create the Scene
	mSectorView = new SectorView(_sceneManager);
	mSectorView->createView(_sectorName);

	//DEBUG memory monitoring
	/*for(int i = 0; i < 10000; ++i)
	{
		mCurrentSector->instantiateObjects(GameSettings::getInstance().getSector(_sectorName), mSceneManager, mDynamicWorld);
		//mCurrentSector->addShip(getNextId(), mSceneManager, mDynamicWorld, "FirstShip", Ogre::Quaternion::IDENTITY, Ogre::Vector3::ZERO);
		delete mCurrentSector;
		mCurrentSector = new Sector();
	}*/

	//instantiate sector objects
	mCurrentSector->instantiateObjects(_sectorName);
}

void SectorController::addInput(const RakNet::RakNetGUID& _id, SectorTick _tick, const InputState& _clientInput)
{
	//TODO retrieve the sector the client is in
	//Add input in this sector input history

	//For now:
	mCurrentSector->addInput(_id, _tick, _clientInput);
}