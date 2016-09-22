#ifndef _SHOT_H_
#define _SHOT_H_

#include "model/SectorObject.h"

#include "model/deserialized/ShotsSettings.h"

class Shot : public SectorObject
{
public:
	Shot(const ShotSettings* _shotSettings, Ogre::SceneManager* _sceneManager) 
		: SectorObject(_shotSettings, _sceneManager),
		mSpeed(Ogre::Vector3::ZERO),
		mTimeElapsed(0.f) 
	{}

	virtual void instantiateObject();

	Ogre::Vector3 mSpeed;

	//Time elapsed since creation
	float mTimeElapsed;
	float getLifeTime() const { return mLifeTime; }

	~Shot();

protected:
	float mLifeTime;
};

#endif //_SHOT_H_