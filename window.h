#pragma once

#include <string>
#include <Windows.h>

namespace Window
{
	void Initalize(void);
	bool Run(void);
	void Finalize(void);

	const unsigned int & width(void);
	const unsigned int & height(void);

	template<class _Type> const _Type width(void) { return static_cast<_Type>(width()); }
	template<class _Type> const _Type height(void) { return static_cast<_Type>(height()); }

	HWND & hwnd(void);

	bool MessageLoop(void);
	LRESULT __stdcall WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
}