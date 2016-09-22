#ifndef _PLAYER_CAMERA_H_
#define _PLAYER_CAMERA_H_

#include <string>

namespace Ogre
{
	class SceneManager;
	class Camera;
	class SceneNode;
	class RenderWindow;
}

class PlayerCamera
{
public:
	///Singleton
	static PlayerCamera& getInstance();

	void init(Ogre::SceneManager* _sceneManager, Ogre::RenderWindow* _renderWindow);
	void createCamera(const std::string& _name);

	Ogre::SceneNode* getCameraNode() const { return mCameraNode; }
	Ogre::Camera* getCamera() const { return mCamera; }

protected:
	///Singleton
	static PlayerCamera* mInstance;
	PlayerCamera() 
		: mCameraNode(NULL),
		mCamera(NULL),
		mRenderWindow(NULL),
		mSceneManager(NULL)
	{}

	Ogre::RenderWindow* mRenderWindow;
	Ogre::SceneManager* mSceneManager;
	Ogre::Camera* mCamera;
	Ogre::SceneNode* mCameraNode;
};

#endif //_PLAYER_CAMERA_H_