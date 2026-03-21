#ifndef INPUT_H
#define INPUT_H

#include "Common.h"
#include "ServiceProvider.h"

const unsigned int CURRENT_STATE = 0;
const unsigned int LAST_STATE = 1;

const unsigned int KEY_COUNT = 256;
const unsigned int MOUSE_BUTTON_COUNT = 3;

const unsigned int KEY_ESCAPE = 0x1B;
const unsigned int KEY_0 = 0x30;
const unsigned int KEY_1 = 0x31;
const unsigned int KEY_2 = 0x32;
const unsigned int KEY_3 = 0x33;
const unsigned int KEY_4 = 0x34;
const unsigned int KEY_5 = 0x35;
const unsigned int KEY_6 = 0x36;
const unsigned int KEY_7 = 0x37;
const unsigned int KEY_8 = 0x38;
const unsigned int KEY_9 = 0x39;
const unsigned int KEY_A = 0x41;
const unsigned int KEY_B = 0x42;
const unsigned int KEY_C = 0x43;
const unsigned int KEY_D = 0x44;
const unsigned int KEY_E = 0x45;
const unsigned int KEY_F = 0x46;
const unsigned int KEY_G = 0x47;
const unsigned int KEY_H = 0x48;
const unsigned int KEY_I = 0x49;
const unsigned int KEY_J = 0x4A;
const unsigned int KEY_K = 0x4B;
const unsigned int KEY_L = 0x4C;
const unsigned int KEY_M = 0x4D;
const unsigned int KEY_N = 0x4E;
const unsigned int KEY_O = 0x4F;
const unsigned int KEY_P = 0x50;
const unsigned int KEY_Q = 0x51;
const unsigned int KEY_R = 0x52;
const unsigned int KEY_S = 0x53;
const unsigned int KEY_T = 0x54;
const unsigned int KEY_U = 0x55;
const unsigned int KEY_V = 0x56;
const unsigned int KEY_W = 0x57;
const unsigned int KEY_X = 0x58;
const unsigned int KEY_Y = 0x59;
const unsigned int KEY_Z = 0x5A;
const unsigned int KEY_NUMPAD0 = 0x60;
const unsigned int KEY_NUMPAD1 = 0x61;
const unsigned int KEY_NUMPAD2 = 0x62;
const unsigned int KEY_NUMPAD3 = 0x63;
const unsigned int KEY_NUMPAD4 = 0x64;
const unsigned int KEY_NUMPAD5 = 0x65;
const unsigned int KEY_NUMPAD6 = 0x66;
const unsigned int KEY_NUMPAD7 = 0x67;
const unsigned int KEY_NUMPAD8 = 0x68;
const unsigned int KEY_NUMPAD9 = 0x69;
const unsigned int KEY_RETURN = 0x0D;
const unsigned int KEY_SPACE = 0x20;
const unsigned int KEY_TAB = 0x09;
const unsigned int KEY_CONTROL = 0x11;
const unsigned int KEY_SHIFT = 0x10;
const unsigned int KEY_ALT = 0x12;
const unsigned int KEY_CAPS = 0x14;
const unsigned int KEY_LEFT = 0x25;
const unsigned int KEY_UP = 0x26;
const unsigned int KEY_RIGHT = 0x27;
const unsigned int KEY_DOWN = 0x28;

const unsigned int MOUSE_BUTTON_LEFT = 0;
const unsigned int MOUSE_BUTTON_MIDDLE = 1;
const unsigned int MOUSE_BUTTON_RIGHT = 2;

class SD_API Input : public IService<Input>
{
public:
	Input() = default;
	~Input() = default;

	bool IsPersistance() override 
	{
		return true;
	};

	bool KeyDown(unsigned int key) const;
	bool KeyJustDown(unsigned int key) const;
	bool KeyJustUp(unsigned int key) const;
	bool MouseButtonDown(unsigned int button) const;
	bool MouseButtonJustDown(unsigned int button) const;
	bool MouseButtonJustUp(unsigned int button) const;
	int MouseX() const;
	int MouseY() const;
	int MouseDeltaX() const;
	int MouseDeltaY() const;

	void Process();
	void SetKey(unsigned int key, bool value);
	void SetMouseButton(unsigned int button, bool value);
	void SetMousePosition(int x, int y);
	void SetMouseLastPosition(int x, int y);

private:
	bool mKeys[2][KEY_COUNT] {};
	bool mMouseButtons[2][MOUSE_BUTTON_COUNT] {};
	int mMouseButtonX[2] {};
	int mMouseButtonY[2] {};
};

#define GetInput() ServiceProvider::Instance()->GetService<Input>()

#endif