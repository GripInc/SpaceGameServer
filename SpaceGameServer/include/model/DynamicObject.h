#ifndef _DYNAMIC_OBJECT_H_
#define _DYNAMIC_OBJECT_H_

#include "model/StaticObject.h"

#include "model/deserialized/DynamicObjectSettings.h"
#include "manager/StateManager.h"

namespace RakNet
{
	class BitStream;
}

class DynamicObject : public StaticObject
{
public:

	DynamicObject() 
		: mUniqueId(0)
	{}
	DynamicObject(const DynamicObjectSettings* _dynamicObjectSettings, Ogre::SceneManager* _sceneManager, btDiscreteDynamicsWorld* _dynamicWorld, UniqueId _uniqueId) : StaticObject(_dynamicObjectSettings, _sceneManager, _dynamicWorld), mUniqueId(_uniqueId) {}

	virtual void destroy();

	virtual btVector3 getInertia() const
	{
		const DynamicObjectSettings* dynamicObjectSettings = static_cast<const DynamicObjectSettings*>(mObjectSettings);
		return dynamicObjectSettings->mLocalInertia;
	}

	virtual void serialize(RakNet::BitStream& _bitStream) const;

	//States management
	virtual void saveState(SectorTick _tick);
	//When set a state, all oldest states are discarded
	virtual void setState(SectorTick _tick);

protected:
	virtual void instantiateCollisionObject();

	UniqueId mUniqueId;

private:
	StateManager<DynamicObjectState> mStateManager;
};

#endif //_DYNAMIC_OBJECT_H_