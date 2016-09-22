#include "model/DynamicObject.h"

#include "model/deserialized/DynamicObjectSettings.h"

#include "BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h"
#include "BulletDynamics/Dynamics/btRigidBody.h"
#include "BulletCollision/CollisionShapes/btCompoundShape.h"

#include "BitStream.h"

#include "utils/OgreBulletConvert.h"

void DynamicObject::instantiateCollisionObject()
{
	if(mCompoundShape != NULL)
	{
		const DynamicObjectSettings* dynamicObjectSettings = static_cast<const DynamicObjectSettings*>(mObjectSettings);

		btTransform startTransform = btTransform(convert(dynamicObjectSettings->mInitialOrientation), convert(dynamicObjectSettings->mInitialPosition));
		mMyMotionState = new MyMotionState(mSceneNode, startTransform);
		mRigidBody = createRigidBody(startTransform, mCompoundShape, mMyMotionState, dynamicObjectSettings->mMass, getInertia());
		mRigidBody->setRestitution(DEFAULT_RESTITUTION_VALUE);
		mRigidBody->setActivationState(DISABLE_DEACTIVATION);
		mRigidBody->setDamping(dynamicObjectSettings->mLinearDamping, dynamicObjectSettings->mAngularDamping);
		mRigidBody->setCollisionFlags(mRigidBody->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK); //CF_CUSTOM_MATERIAL_CALLBACK allows the trigger of MyContactCallback on contact added
		mDynamicWorld->addRigidBody(mRigidBody);
	}
}

void DynamicObject::serialize(RakNet::BitStream& _bitStream) const
{
	_bitStream.Write(mUniqueId);

	DynamicObjectState dynamicObjectState(mRigidBody);
	dynamicObjectState.serialize(_bitStream);
}

void DynamicObject::destroy()
{
	//StaticObject::destroy();
}

void DynamicObject::saveState(SectorTick _tick)
{
	mStateManager.saveState(_tick, DynamicObjectState(mRigidBody));
}

void DynamicObject::setState(SectorTick _tick)
{
	DynamicObjectState dynamicObjectState;
	mStateManager.getState(_tick, dynamicObjectState);

	mRigidBody->setWorldTransform(dynamicObjectState.mWorldTransform);
	mRigidBody->setLinearVelocity(dynamicObjectState.mLinearVelocity);
	mRigidBody->setAngularVelocity(dynamicObjectState.mAngularVelocity);
	mRigidBody->applyCentralForce(dynamicObjectState.mTotalForce);
	mRigidBody->applyTorque(dynamicObjectState.mTotalTorque);
}