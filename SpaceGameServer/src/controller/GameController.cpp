#include "controller/GameController.h"
#include "model/ObjectPart.h"
#include "model/GameSettings.h"
#include "model/PlayersData.h"
#include "controller/InputController.h"
#include "controller/UIController.h"
#include "controller/CameraController.h"
#include "controller/SectorController.h"
#include "view/PlayerCamera.h"

#include "OgreRoot.h"

#include "utils/StringUtils.h"
#include "utils/OgreBulletConvert.h"
#include "utils/OgreUtils.h"
#include "utils/BulletDebugDraw.h"

#include "network/NetworkService.h"

#include "manager/LoggerManager.h"

namespace
{
	const std::string LOG_CLASS_TAG = "GameController";
}

const float GameController::mDebugPanelRefreshRate = 0.1f;
const float GameController::GAME_UPDATE_RATE = 1.f / 30.f;

/** Init */
void GameController::init(const std::string& _playerDataFilePath, const std::string& _gameSettingsFilePath, Ogre::Root* _root, Ogre::RenderWindow* _renderWindow, Ogre::SceneManager* _sceneManager, NetworkLayer& _networkLayer)
{
	NetworkService::getInstance().init(&_networkLayer, this);
	_networkLayer.registerNetworkService(&NetworkService::getInstance());

	mRenderWindow = _renderWindow;

	mRoot = _root;
	mRoot->addFrameListener(this);
	
	mSceneManager = _sceneManager;

	//Init camera
	PlayerCamera::getInstance().init(mSceneManager, mRenderWindow);

	//Create input controller
	mInputController = InputController::getSingletonPtr();
	mInputController->initialise(mRenderWindow);

	//Read game xml files
	GameSettings::getInstance().init(_gameSettingsFilePath);

	//Read players data file
	if(!PlayersData::getInstance().init(_playerDataFilePath))
	{
		//TODO error loading players data
	}

	//DEBUG
	mDebugPanelLastRefresh = 0.f;
	mLaggyValue = 10000;
}

void GameController::addPlayer(const RakNet::RakNetGUID& _id, PlayerData* _playerData)
{
	LoggerManager::getInstance().logI(LOG_CLASS_TAG, "addPlayer", "Sending player data for player " + _playerData->mPlayerId + " GUID:" + std::string(_id.ToString()), false);

	mConnectedPlayers.insert(std::pair<RakNet::RakNetGUID, PlayerData*>(_id, _playerData));
}

const PlayerData* GameController::getPlayerData(const RakNet::RakNetGUID& _clientId) const
{
	std::map<RakNet::RakNetGUID, PlayerData*>::const_iterator foundPlayer = mConnectedPlayers.find(_clientId);
	if(foundPlayer != mConnectedPlayers.end())
	{
		return (*foundPlayer).second;
	}

	LoggerManager::getInstance().logE(LOG_CLASS_TAG, "getPlayerData", "No player data found for " + std::string(_clientId.ToString()));
	return NULL;
}

PlayerData* GameController::getPlayerData(const RakNet::RakNetGUID& _clientId)
{
	std::map<RakNet::RakNetGUID, PlayerData*>::iterator foundPlayer = mConnectedPlayers.find(_clientId);
	if(foundPlayer != mConnectedPlayers.end())
	{
		return (*foundPlayer).second;
	}

	LoggerManager::getInstance().logE(LOG_CLASS_TAG, "getPlayerData", "No player data found for " + std::string(_clientId.ToString()));
	return NULL;
}

void GameController::startGame()
{
	LoggerManager::getInstance().logI(LOG_CLASS_TAG, "startGame", "", false);

	//Link gameController to input controller
	mInputController->addKeyListener(this, "GameController"); //May be removed in the future if not needed

	//UIController
	mUIController = new UIController(this, mRenderWindow, mInputController->getMouse()); //For now, displays debug panel only

	//Link uiController to input controller (useless for now)
	mInputController->addKeyListener(mUIController, "UIController");
	mInputController->addMouseListener(mUIController, "UIController");

	//Init sector controller
	mSectorController = new SectorController();

	//Create sector
	mSectorController->createSector("Alpha", mSceneManager, GAME_UPDATE_RATE, MAX_SECTOR_TICK_REWIND_AMOUNT);

	//Create player camera controller
	mCameraController = new CameraController(PlayerCamera::getInstance().getCamera());
	//Link mCameraController to input controller
	mInputController->addKeyListener(mCameraController, "CameraController");
	mInputController->addMouseListener(mCameraController, "CameraController");
	mRoot->addFrameListener(mCameraController);
}

bool GameController::frameRenderingQueued(const Ogre::FrameEvent& evt)
{
	//DeltaTime
	if(evt.timeSinceLastFrame == 0.f)
		return true;

	mGameUpdateAccumulator += evt.timeSinceLastFrame;
	
	mLoopTimer.reset();
	unsigned long loopDurationTime = mLoopTimer.getMilliseconds();
	while(mGameUpdateAccumulator > GAME_UPDATE_RATE)
	{
		LoggerManager::getInstance().logI(LOG_CLASS_TAG, "frameRenderingQueued", "Entering update loop.", false);

		//Network
		//Write input in mClientsInput if any in buffer
		//Receive launch requests
		NetworkService::getInstance().processNetworkBuffer();

		if(mSectorController)
		{
			//Sector update for next sector tick
			mSectorController->updateSector();
		}
		
		//Capture input and pass it to all registered controllers
		mInputController->capture();

		mGameUpdateAccumulator -= GAME_UPDATE_RATE;
	}
	loopDurationTime = mLoopTimer.getMilliseconds() - loopDurationTime;

	if (loopDurationTime > GAME_UPDATE_RATE * 1000)
		LoggerManager::getInstance().logW(LOG_CLASS_TAG, "frameRenderingQueued", "loopDurationTime was " + StringUtils::toStr(loopDurationTime) + " : Simulation is getting late!");

	//DEBUG PANEL
	if(mSectorController)
	{
		if(mUIController->getDebugPanel()->getAllParamNames().size() == 0)
		{
			Ogre::StringVector paramNames;
			paramNames.push_back("");
			paramNames.push_back("");
			paramNames.push_back("");
			paramNames.push_back("");
			paramNames.push_back("");
			paramNames.push_back("");
			paramNames.push_back("");
			paramNames.push_back("");
			paramNames.push_back("");
			paramNames.push_back("");
			paramNames.push_back("");
			paramNames.push_back("");
			paramNames.push_back("");
			paramNames.push_back("");

			mUIController->getDebugPanel()->setAllParamNames(paramNames);
		}
		mDebugPanelLastRefresh += evt.timeSinceLastFrame;
		if(mDebugPanelLastRefresh > mDebugPanelRefreshRate)
		{
			mUIController->getDebugPanel()->setParamValue(0, StringUtils::toStr(""));
			mUIController->getDebugPanel()->setParamValue(1, StringUtils::toStr(""));
			mUIController->getDebugPanel()->setParamValue(2, StringUtils::toStr(""));
			mUIController->getDebugPanel()->setParamValue(3, StringUtils::toStr(""));
			mUIController->getDebugPanel()->setParamValue(4, StringUtils::toStr(""));
			mUIController->getDebugPanel()->setParamValue(5, StringUtils::toStr(""));
			mUIController->getDebugPanel()->setParamValue(6, StringUtils::toStr(""));
			mUIController->getDebugPanel()->setParamValue(7, StringUtils::toStr(""));
			mUIController->getDebugPanel()->setParamValue(8, StringUtils::toStr(""));
			mUIController->getDebugPanel()->setParamValue(9, StringUtils::toStr(""));
			mUIController->getDebugPanel()->setParamValue(10, StringUtils::toStr(""));
			mUIController->getDebugPanel()->setParamValue(11, StringUtils::toStr(""));
			mUIController->getDebugPanel()->setParamValue(12, StringUtils::toStr(""));
			mUIController->getDebugPanel()->setParamValue(13, StringUtils::toStr(""));
			mDebugPanelLastRefresh = 0.f;
		}
	}

	//DEBUG
	for(int i = 0; i < mLaggyValue; i++)
	{
		mLaggyValue = mLaggyValue;
	}

	return true;
}

//TODO move to UIController
/// User entries handling ///
bool GameController::keyPressed( const OIS::KeyEvent &arg )
{
	return true;
}

//TODO move to UIController
bool GameController::keyReleased( const OIS::KeyEvent &arg )
{
	switch(arg.key)
	{
		case OIS::KC_F1:
			mSectorController->switchDisplayDebug();
			break;
		case OIS::KC_F2:
			mSectorController->switchDisplay();
			break;
		case OIS::KC_F3:
			mLaggyValue *= 2;
			break;
		case OIS::KC_F4:
			mLaggyValue /= 2;
			break;
		default:break;
	}

    return true;
}

void GameController::instantiateClientShip(const RakNet::RakNetGUID& _clientId, std::string& _outSector, Ogre::Vector3& _outPosition, Ogre::Quaternion& _outOrientation, UniqueId& _shipUniqueId, SectorTick& _sectorTick)
{
	//TODO random launchpoint among some
	//TODO queue players if all points are busy

	PlayerData* playerData = getPlayerData(_clientId);

	if(playerData)
	{
		const StationSettings* stationSettings = GameSettings::getInstance().getStation(playerData->mLastStation);
		if(stationSettings)
		{
			_outPosition = stationSettings->mLaunchPoints[0].mInitialPosition;
			_outOrientation = stationSettings->mLaunchPoints[0].mInitialOrientation;
			_outSector = playerData->mLastSector;

			const std::string& shipId = playerData->mShipId;

			mSectorController->instantiateClientShip(_clientId, playerData->mPlayerShip, shipId, _outPosition, _outOrientation, _shipUniqueId, _sectorTick);
		}
		else
		{
			LoggerManager::getInstance().logE(LOG_CLASS_TAG, "instantiateClientShip", "No stationSettings found for station " + playerData->mLastStation);
			assert(false);
		}
	}
	else
	{
		LoggerManager::getInstance().logE(LOG_CLASS_TAG, "instantiateClientShip", "No player data found for GUID " + std::string(_clientId.ToString()));
		assert(false);
	}
}

void GameController::addInput(const RakNet::RakNetGUID& _id, SectorTick _tick, const InputState& _clientInput)
{
	mSectorController->addInput(_id, _tick, _clientInput);
}