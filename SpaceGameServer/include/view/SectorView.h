#ifndef _SECTOR_VIEW_H_
#define _SECTOR_VIEW_H_

#include <string>

namespace Ogre
{
	class Camera;
	class Root;
	class SceneManager;
	class RenderWindow;
	class SceneNode;
}

class SectorView
{
public:
	SectorView(Ogre::SceneManager* _sceneManager)
		: mSceneManager(_sceneManager)
	{}

	~SectorView();

	void createView(const std::string& _sectorName);

protected:
	//Used for cleaning
	Ogre::SceneManager* mSceneManager;
};

#endif //_SECTOR_VIEW_H_