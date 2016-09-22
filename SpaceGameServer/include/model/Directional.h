#ifndef _DIRECTIONAL_H_
#define _DIRECTIONAL_H_

#include <string>

class DirectionalSettings;

class Directional
{
public:
	Directional() : mName(""), mSpace(0), mHitPoints(0), mMass(0), mTurnRateMultiplier(0.f), mInertiaMultiplier(0.f) {}
	void init(const DirectionalSettings& _settings);

	float getInertiaMultiplier() const { return mInertiaMultiplier; }
	float getTurnRateMultiplier() const { return mTurnRateMultiplier; }

protected:
	std::string mName;
	unsigned int mSpace;
	unsigned int mHitPoints;
	unsigned int mMass;
	float mTurnRateMultiplier;
	float mInertiaMultiplier;
};

#endif //_DIRECTIONAL_H_