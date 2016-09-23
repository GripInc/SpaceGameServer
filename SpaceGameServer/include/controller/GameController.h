#ifndef _GAME_CONTROLLER_H_
#define _GAME_CONTROLLER_H_

#include "OgreFrameListener.h"
#include "OgreVector3.h"
#include "OgreTimer.h"

#include "OIS.h"

#include <string>

#include "model/PlayerData.h"
#include "model/InputState.h"

#include "RakNetTypes.h"
#include "SpaceGameTypes.h"

class InputController;
class UIController;
class SectorController;
class StationController;
class NetworkService;
class NetworkLayer;
class CameraController;

namespace Ogre
{
	class RenderWindow;
	class Root;
	class Camera;
	class SceneNode;
}

class GameController : public Ogre::FrameListener, public OIS::KeyListener
{
public:
	GameController()
		: mSectorController(NULL),
		mCameraController(NULL),
		mGameUpdateAccumulator(0.f)
	{}

	void init(const std::string& _playerDataFilePath, const std::string& _gameSettingsFilePath, Ogre::Root* _root, Ogre::RenderWindow* _renderWindow, Ogre::SceneManager* _sceneManager, NetworkLayer& _networkLayer);
	void startGame();

	void addPlayer(const RakNet::RakNetGUID& _id, PlayerData* _playerData);
	const PlayerData* getPlayerData(const RakNet::RakNetGUID& _clientId) const;
	PlayerData* getPlayerData(const RakNet::RakNetGUID& _clientId);

	void instantiateClientShip(const RakNet::RakNetGUID& _clientId, std::string& _outSector, Ogre::Vector3& _outPosition, Ogre::Quaternion& _outOrientation, UniqueId& _shipUniqueId, SectorTick& _sectorTick);

	//Add input for a client in a sector
	void addInput(const RakNet::RakNetGUID& _id, SectorTick _tick, const InputState& _clientInput);

protected:
	static const float GAME_UPDATE_RATE;
	float mGameUpdateAccumulator;
	Ogre::Timer mLoopTimer;

	Ogre::RenderWindow* mRenderWindow;
	Ogre::Root* mRoot;

	Ogre::SceneManager* mSceneManager;

	SectorController* mSectorController;
	InputController* mInputController;
	UIController* mUIController;

	CameraController* mCameraController;

	// Ogre::FrameListener
    bool frameRenderingQueued(const Ogre::FrameEvent& evt);

	//DEBUG
	float mDebugPanelLastRefresh;
	static const float sDebugPanelRefreshRate;
	
	virtual bool keyPressed( const OIS::KeyEvent &arg );
    virtual bool keyReleased( const OIS::KeyEvent &arg );

	long mLaggyValue;

	//Connected players
	std::map<RakNet::RakNetGUID, PlayerData*> mConnectedPlayers;
};

#endif //_GAME_CONTROLLER_H_