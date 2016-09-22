#include "model/SectorObject.h"

#include "model/deserialized/SectorObjectSettings.h"

#include "OgreSceneNode.h"
#include "OgreSceneManager.h"
#include "OgreEntity.h"
#include "OgreMesh.h"

#include "utils/StringUtils.h"
#include "utils/XMLHelper.h"
#include "utils/OgreUtils.h"
#include "utils/OgreBulletConvert.h"

void SectorObject::instantiateObject()
{
	instantiateObjectSceneNode(mObjectSettings->mInitialOrientation, mObjectSettings->mInitialPosition, mObjectSettings->mInitialScale, mObjectSettings->mMesh, mObjectSettings->mName, mSceneManager);
}

void SectorObject::instantiateObjectSceneNode(const Ogre::Quaternion& _orientation, const Ogre::Vector3& _position, const Ogre::Vector3& _scale, const std::string& _mesh, const std::string& _name, Ogre::SceneManager* _sceneManager)
{
	mSceneManager = _sceneManager;
	mName = _name;

	if(mIsinstantiated)
	{
		//TODO wtf
		std::vector<int> makeCrash;
		makeCrash[1];
	}

	//Create ogre scene node
	mSceneNode = _sceneManager->getRootSceneNode()->createChildSceneNode();

	//Create ogre entity and attach entity to scene node
	mSceneNode->attachObject(createEntity(_name, _mesh, _scale, _sceneManager));

	//Set up start pos and orientation
	mSceneNode->setPosition(_position);
	mSceneNode->setOrientation(_orientation);
	mSceneNode->setScale(_scale);

	mIsinstantiated = true;
}

Ogre::Entity* SectorObject::createEntity(const std::string& _name, const std::string& _mesh, const Ogre::Vector3& _scale, Ogre::SceneManager* _sceneManager)
{
	Ogre::Entity* newEntity = NULL;
	if(_mesh.empty())
	{
		newEntity = _sceneManager->createEntity(Ogre::SceneManager::PT_CUBE);

		//DEBUG
		//mEntity->setMaterialName("Examples/SphereMappedRustySteel");
		newEntity->setMaterialName("test/GrassFloor");
	}
	else
	{
		newEntity = _sceneManager->createEntity(_mesh);
	}

	return newEntity;
}

void SectorObject::addSubSceneNode(const Ogre::Quaternion& _orientation, const Ogre::Vector3& _position, const Ogre::Vector3& _scale, const std::string& _mesh, const std::string& _name, Ogre::SceneManager* _sceneManager)
{
	if(!mIsinstantiated)
	{
		//TODO wtf
		std::vector<int> makeCrash;
		makeCrash[1];
	}

	//Create ogre scene node
	Ogre::SceneNode* newSceneNode = mSceneNode->createChildSceneNode();

	//Create ogre entity and attach entity to scene node
	newSceneNode->attachObject(createEntity(_name, _mesh, _scale, _sceneManager));

	//Set up start pos and orientation
	newSceneNode->setPosition(_position);
	newSceneNode->setOrientation(_orientation);
	newSceneNode->setScale(_scale);
}

void SectorObject::destroy()
{
	OgreUtils::destroySceneNode(mSceneNode);

	mSceneNode = NULL;
	mEntity = NULL;
}

SectorObject::~SectorObject()
{
	
}

void SectorObject::setVisible(bool _value)
{ 
	mSceneNode->setVisible(_value);
}

Ogre::Vector3 SectorObject::getRelativePosition(const Ogre::Vector3& _originalPosition)
{
	return mSceneNode->getOrientation() * _originalPosition + mSceneNode->getPosition();
}