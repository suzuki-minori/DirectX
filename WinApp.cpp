#include "WinApp.h"
#include<cassert>


void WinApi::Initialize()
{
	HRESULT hr= CoInitializeEx(0, COINIT_MULTITHREADED);

	//WNDCLASS wc{};
	wc.lpfnWndProc = WindowProc;
	wc.lpszClassName = L"CG2WindowClass";
	wc.hInstance = GetModuleHandle(nullptr);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

	RegisterClass(&wc);

	//const int32_t kClientWidth = 1280;
	//const int32_t kClientHeight = 720;

	RECT wrc = { 0,0,kClientWidth,kClientHeight };

	AdjustWindowRect(&wrc, WS_EX_OVERLAPPEDWINDOW, false);

	/*HWND*/ hwnd = CreateWindow(
		wc.lpszClassName,
		L"CG2",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		wrc.right - wrc.left,
		wrc.bottom - wrc.top,
		nullptr,
		nullptr,
		wc.hInstance,
		nullptr);

	ShowWindow(hwnd, SW_SHOW);


}

void WinApi::Update()
{
}

void WinApi::Finalize()
{
	CloseWindow(hwnd);
	CoUninitialize();
}

//
LRESULT CALLBACK WinApi::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)){
		return true;
	}

	//
	switch (msg) {
		//
	case WM_DESTROY:
		//
		PostQuitMessage(0);
		return 0;
	}

	//
	return DefWindowProc(hwnd, msg, wparam, lparam);

}