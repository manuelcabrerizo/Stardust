#include "Input.h"

bool Input::KeyDown(unsigned int key) const
{
	return mKeys[CURRENT_STATE][key];
}

bool Input::KeyJustDown(unsigned int key) const
{
	return mKeys[CURRENT_STATE][key] && !mKeys[LAST_STATE][key];
}

bool Input::KeyJustUp(unsigned int key) const
{
	return !mKeys[CURRENT_STATE][key] && mKeys[LAST_STATE][key];
}

bool Input::MouseButtonDown(unsigned int button) const
{
	return mMouseButtons[CURRENT_STATE][button];
}

bool Input::MouseButtonJustDown(unsigned int button) const
{
	return mMouseButtons[CURRENT_STATE][button] && !mMouseButtons[LAST_STATE][button];
}

bool Input::MouseButtonJustUp(unsigned int button) const
{
	return !mMouseButtons[CURRENT_STATE][button] && mMouseButtons[LAST_STATE][button];
}

int Input::MouseX() const
{
	return mMouseButtonX[CURRENT_STATE];
}

int Input::MouseY() const
{
	return mMouseButtonY[CURRENT_STATE];
}

int Input::MouseDeltaX() const
{
	return mMouseButtonX[CURRENT_STATE] - mMouseButtonX[LAST_STATE];
}

int Input::MouseDeltaY() const
{
	return mMouseButtonY[CURRENT_STATE] - mMouseButtonY[LAST_STATE];
}

void Input::Process()
{
	memcpy(mKeys[LAST_STATE], mKeys[CURRENT_STATE], KEY_COUNT * sizeof(bool));
	memcpy(mMouseButtons[LAST_STATE], mMouseButtons[CURRENT_STATE], MOUSE_BUTTON_COUNT * sizeof(bool));
	mMouseButtonX[LAST_STATE] = mMouseButtonX[CURRENT_STATE];
	mMouseButtonY[LAST_STATE] = mMouseButtonY[CURRENT_STATE];	
}

void Input::SetKey(unsigned int key, bool value)
{
	mKeys[CURRENT_STATE][key] = value;
}

void Input::SetMouseButton(unsigned int button, bool value)
{
	mMouseButtons[CURRENT_STATE][button] = value;
}

void Input::SetMousePosition(int x, int y)
{
	mMouseButtonX[CURRENT_STATE] = x;
	mMouseButtonY[CURRENT_STATE] = y;
}

void Input::SetMouseLastPosition(int x, int y)
{
	mMouseButtonX[LAST_STATE] = x;
	mMouseButtonY[LAST_STATE] = y;
}