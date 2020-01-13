#pragma once

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

class Window {
	HWND hWnd = nullptr;
	HINSTANCE hInstance = nullptr;
	int Width = 0;
	int Height = 0;

	bool ProcMsg() {
		MSG msg;
		bool IsActive = true;
		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) IsActive = false;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		return IsActive;
	}
	virtual bool Init() {
		return true;
	}
	virtual void Term() {
	}
	virtual void Update() {
	}
public:
	void Start() {
		if(Init()) {
			while(ProcMsg()) {
				Update();
			}
		}
		Term();
	}
	int GetWidth() {
		return Width;
	}
	int GetHeight() {
		return Height;
	}
	HWND GetWindowHandle() {
		return hWnd;
	}
	
	HINSTANCE GetInstance() {
		return hInstance;
	}

	virtual ~Window() {
	}

	Window(const char *appname, int w, int h) {
		Width = w;
		Height = h;
		auto WindowProc = [](HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT {
			int temp = wParam & 0xFFF0;
			switch (msg) {
			case WM_SYSCOMMAND:
				if (temp == SC_MONITORPOWER || temp == SC_SCREENSAVE) {
					return 0;
				}
				break;
			case WM_IME_SETCONTEXT:
				lParam &= ~ISC_SHOWUIALL;
				return 0;
				break;
			case WM_CLOSE:
			case WM_DESTROY:
				PostQuitMessage(0);
				return 0;
				break;
			case WM_KEYDOWN:
				if (wParam == VK_ESCAPE) PostQuitMessage(0);
				break;
			case WM_SIZE:
				break;
			}
			return DefWindowProc(hWnd, msg, wParam, lParam);
		};
		hInstance = GetModuleHandle(NULL);
		static WNDCLASSEX wcex = {
			sizeof(WNDCLASSEX), CS_CLASSDC | CS_VREDRAW | CS_HREDRAW,
			WindowProc, 0L, 0L, GetModuleHandle(NULL), LoadIcon(NULL, IDI_APPLICATION),
			LoadCursor(NULL, IDC_ARROW), (HBRUSH)GetStockObject(BLACK_BRUSH),
			NULL, appname, NULL
		};
		wcex.hInstance = hInstance;
		RegisterClassEx(&wcex);

		DWORD styleex = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		DWORD style = WS_OVERLAPPEDWINDOW;
		style &= ~(WS_SIZEBOX | WS_MAXIMIZEBOX | WS_MINIMIZEBOX);

		RECT rc;
		SetRect( &rc, 0, 0, Width, Height );
		AdjustWindowRectEx( &rc, style, FALSE, styleex);
		rc.right -= rc.left;
		rc.bottom -= rc.top;
		hWnd = CreateWindowEx(styleex, appname, appname, style, 0, 0, rc.right, rc.bottom, NULL, NULL, wcex.hInstance, NULL);
		ShowWindow(hWnd, SW_SHOW);
		UpdateWindow(hWnd);
		ShowCursor(TRUE);
	}
};


