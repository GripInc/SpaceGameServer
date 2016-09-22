#ifndef _ENGINE_H_
#define _ENGINE_H_

#include <string>

class EngineSettings;

class Engine
{
public:
	Engine() : mName(""), mReactivity(0.f), mPower(0.f), mMaxSpeed(0.f), mThrustSensitivity(0.f), mThrustMaxValue(0.f), mWantedThrust(0.f), mRealThrust(0.f) {}

	void init(const EngineSettings& _engineSettings);

	///Getters
	const std::string& getName() const { return mName; }
	float getReactivity() const { return mReactivity; }
	float getPower() const { return mPower; }
	float getMaxSpeed() const { return mMaxSpeed; }
	float getThrustSensitivity() const { return mThrustSensitivity; }
	float getThrustMaxValue() const { return mThrustMaxValue; }

	float mWantedThrust;
	float mRealThrust;

protected:
	std::string mName;
	///The rate real thrust approach wanted thrust
	float mReactivity;
	///
	float mPower;
	///Max gaz ejection speed
	float mMaxSpeed;
	///The rate the player can add power in engine
	float mThrustSensitivity;
	float mThrustMaxValue;
};

#endif //_SHIP_ENGINE_H_