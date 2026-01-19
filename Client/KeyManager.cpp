#include "stdafx.h"
#include "KeyManager.h"


UINT gKeyValue[(UINT)KEY::KEY_END]
=
{
	'W', 'S', 'A', 'D',
	'Z', 'X', 'C', 'V',
	'R', 'T', 'Y', 'U', 'I', 'O', 'P',

	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',

	0x30, 0x31, 0x32, 0x33, 0x34,
	0x35, 0x36, 0x37, 0x38, 0x39,

	VK_LEFT,
	VK_RIGHT,
	VK_UP,
	VK_DOWN,

	VK_LBUTTON,
	VK_RBUTTON,

	VK_RETURN,
	VK_ESCAPE,
	VK_SPACE,
	VK_LSHIFT, 
	VK_MENU,
	VK_CONTROL,
};

void CKeyManager::Init()
{
	for (UINT i = 0; i < (UINT)KEY::KEY_END; ++i)
		input_vector.emplace_back(KEY_STATE::NONE, false);
}

void CKeyManager::Tick()
{
	if (nullptr == GetFocus())
	{
		for (size_t i = 0; i < input_vector.size(); ++i)
		{
			if (KEY_STATE::TAP == input_vector[i].state || KEY_STATE::PRESSED == input_vector[i].state)
			{
				input_vector[i].state = KEY_STATE::RELEASED;
			}
			else if (KEY_STATE::RELEASED == input_vector[i].state)
			{
				input_vector[i].state = KEY_STATE::NONE;
			}

			input_vector[i].prev_pressed = false;
		}
	}

	else
	{
		for (size_t i = 0; i < input_vector.size(); ++i)
		{
			// KEY 가 눌렸다
			if (GetAsyncKeyState(gKeyValue[i]) & 0x8001)
			{
				// 이전에는 안눌려있었다.
				if (!input_vector[i].prev_pressed)
				{
					input_vector[i].state = KEY_STATE::TAP;
				}
				// 이전에도 눌려있었다.
				else
				{
					input_vector[i].state = KEY_STATE::PRESSED;
				}

				input_vector[i].prev_pressed = true;
			}
			// 해당 KEY 가 안눌려있다.
			else
			{
				if (input_vector[i].prev_pressed)
				{
					// 이전 Frame 에서는 눌려있었다.
					input_vector[i].state = KEY_STATE::RELEASED;
				}
				else
				{
					// 이전에도 안눌려있었고, 지금도 안눌려있다.
					input_vector[i].state = KEY_STATE::NONE;
				}

				input_vector[i].prev_pressed = false;
			}
		}
		 
		// 마우스 좌표 갱신
		prev_mouse_pos = cur_mouse_pos;	

		POINT ptMouse = {};
		GetCursorPos(&ptMouse);
		ScreenToClient(ghWnd, &ptMouse);
		cur_mouse_pos = Vec2((float)ptMouse.x, (float)ptMouse.y);
		drag_dir = cur_mouse_pos - prev_mouse_pos;
		drag_dir.y *= -1.f;
		drag_dir.Normalize();
	}
}
