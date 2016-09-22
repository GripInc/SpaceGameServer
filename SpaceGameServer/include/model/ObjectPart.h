#ifndef _OBJECT_PART_H_
#define _OBJECT_PART_H_

#include <string>

#include "BulletCollision/CollisionShapes/btCollisionShape.h"

class CollisionShapeSettings;
class ObjectPartSettings;

class ObjectPart
{
public:
	ObjectPart() 
		: mName(""),
		mHitPoints(0)
	{}

	void init(const ObjectPartSettings& _objectPartSettings);
	btCollisionShape* createCollisionShape(const CollisionShapeSettings& _collisionShapeSettings);

	const std::string& getName() const { return mName; }

	int mHitPoints;

protected:
	std::string mName;
};

#endif //_SHIP_PART_H_