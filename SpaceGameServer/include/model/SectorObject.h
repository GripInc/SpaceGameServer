#ifndef _SECTOR_OBJECT_H_
#define _SECTOR_OBJECT_H_

#include <string>

#include "OgreMath.h"
#include "OgreVector3.h"

#include "LinearMath/btTransform.h"

//Forward declarations
class btDiscreteDynamicsWorld;

class SectorObjectSettings;

class SectorObject
{
public:
	SectorObject()
		: mEntity(NULL),
		mSceneNode(NULL),
		mSceneManager(NULL),
		mObjectSettings(NULL),
		mIsinstantiated(false)
	{}

	SectorObject(const SectorObjectSettings* _sectorObjectSettings, Ogre::SceneManager* _sceneManager) :
		mEntity(NULL),
		mSceneNode(NULL),
		mSceneManager(_sceneManager),
		mObjectSettings(_sectorObjectSettings),
		mIsinstantiated(false)
	{}

	~SectorObject();

	virtual void instantiateObject();
	
	Ogre::SceneNode* getSceneNode() { return mSceneNode; }
	const std::string& getName() const { return mName; }
	Ogre::Vector3 getRelativePosition(const Ogre::Vector3& _originalPosition);

	void setVisible(bool _value);

	virtual void destroy();

protected:
	//Ogre scene relatives objects
	Ogre::Entity* mEntity;
    Ogre::SceneNode* mSceneNode;
	//Used for destruction
	Ogre::SceneManager* mSceneManager;
	
	const SectorObjectSettings* mObjectSettings;

	std::string mName;

	bool mIsinstantiated;

	//Create entity from either mesh, or cube prefab
	virtual Ogre::Entity* createEntity(const std::string& _name, const std::string& _mesh, const Ogre::Vector3& _scale, Ogre::SceneManager* _sceneManager);

	void addSubSceneNode(const Ogre::Quaternion& _orientation, const Ogre::Vector3& _position, const Ogre::Vector3& _scale, const std::string& _mesh, const std::string& _name, Ogre::SceneManager* _sceneManager);

	void instantiateObjectSceneNode(const Ogre::Quaternion& _orientation, const Ogre::Vector3& _position, const Ogre::Vector3& _scale, const std::string& _mesh, const std::string& _name, Ogre::SceneManager* _sceneManager);
};

#endif //_SECTOR_OBJECT_H_