#pragma once

#include<cstdint>
#include<Windows.h>
#include"externals/imgui/imgui.h"
#include"externals/imgui/imgui_impl_dx12.h"
#include"externals/imgui/imgui_impl_win32.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


class WinApi
{
public:
	//
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	static const int32_t kClientWidth = 1280;
	static const int32_t kClientHeight = 720;


public:
	//
	void Initialize();
	//
	void Update();
	//
	void Finalize();


	//
	HWND GetHwnd()const { return hwnd; }
	//
	HINSTANCE GetInstance()const { return wc.hInstance; }


private:
	//
	HWND hwnd = nullptr;
	//
	WNDCLASS wc{};





};

