#ifndef _INPUT_STATE_H_
#define _INPUT_STATE_H_

class InputState
{
public:
	InputState ()
		: mUpKeyPressed(false),
		mDownKeyPressed(false),
		mLeftKeyPressed(false),
		mRightKeyPressed(false),
		mQKeyPressed(false),
		mAKeyPressed(false),
		mWKeyPressed(false),
		mXKeyPressed(false),
		mSKeyPressed(false),
		mZKeyPressed(false),
		mMouseXAbs(0),
		mMouseYAbs(0),
		mJoystickXAbs(0),
		mJoystickYAbs(0),
		mFirePressed(false)
	{}

	bool mAKeyPressed;
	bool mQKeyPressed;
	bool mWKeyPressed;
	bool mXKeyPressed;
	bool mSKeyPressed;
	bool mZKeyPressed;

	float mJoystickXAbs;
	float mJoystickYAbs;
	float mMouseXAbs;
	float mMouseYAbs;

	bool mUpKeyPressed;
	bool mDownKeyPressed;
	bool mLeftKeyPressed;
	bool mRightKeyPressed;

	bool mFirePressed;
};

#endif //_INPUT_STATE_H_