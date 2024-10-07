#include "Input.h"
#include<cassert>


using namespace Microsoft::WRL;
#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")


void Input::Initialize(HINSTANCE hInstance, HWND hwnd)
{
	HRESULT result;

	ComPtr<IDirectInput8> directInput = nullptr;
	result = DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput, nullptr);
	assert(SUCCEEDED(result));
	
	IDirectInputDevice8* keyboard = nullptr;
	result = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
	assert(SUCCEEDED(result));
	
	result = keyboard->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(result));
	
	result = keyboard->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(result));
	


}

bool Input::PushKey(BYTE keyNumber)
{
	//
	if (key[keyNumber]) {
		return true;
	}

	return false;
}

void Input::Update()
{
	//
	keyboard->Acquire();
	//
	//BYTE key[256] = {};
	keyboard->GetDeviceState(sizeof(key),key);
	//
	if (input->PushKey[DIK_0]) {
		OutputDebugStringA("hit 0\n");
	}
}

