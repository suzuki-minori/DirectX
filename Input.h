#pragma once
#include<Windows.h>
#include<wrl.h>
#define DIRECTINPUT_VERSION 0x0800
#include<dinput.h>

class Input
{

public:

	template <class T>using ComPtr = Microsoft::WRL::ComPtr<T>;
	void Update();
	

public:

	void Initialize(HINSTANCE hInstance, HWND hwnd);

	bool PushKey(BYTE keyNumber);

private:

	ComPtr<IDirectInputDevice8>keyboard;
	BYTE key[256] = {};
};

