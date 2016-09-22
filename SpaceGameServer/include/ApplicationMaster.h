#ifndef _APPLICATION_MASTER_H_
#define _APPLICATION_MASTER_H_

#include "Ogre.h"
#include "controller/GameController.h"
#include "network/NetworkLayer.h"

#define GAME_SETTINGS_FILE_PATH "../SpaceGameRessources/"
#define PLAYERS_DATA_FILE_PATH "../playersData/playersData.json"

class NetworkLayer;

class ApplicationMaster
{
public:
	ApplicationMaster();
    virtual ~ApplicationMaster();

	void startApplication();

protected:
	bool setup();
    bool configure(void);
    void setupResources(void);
    void loadResources(void);

    Ogre::Root *mRoot;
    Ogre::RenderWindow* mWindow;
    Ogre::String mResourcesCfg;
    Ogre::String mPluginsCfg;

    bool mShutDown;

    NetworkLayer mNetworkLayer;
	
	GameController mGameController;
};

#endif //_APPLICATION_MASTER_H_