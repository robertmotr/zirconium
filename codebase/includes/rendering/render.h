#pragma once

#include "pch.h"
#include "globals.h"
#include "mem.h"

// https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_win32.cpp see line 571
// need to forward declare this for some reason
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

/*
* Initializes the ImGui memory table with the desired values read from the game.
*/
bool __stdcall initMemTable();

void __stdcall tableSortHelper();

int __stdcall compareValues(const MEM_TYPES& a, const MEM_TYPES& b);

/*
* Initializes ImGui overlay inside our hook.
* @return S_OK if successful, E_FAIL otherwise
*/
bool __stdcall initOverlay(ID3D11Device* device, ID3D11DeviceContext* deviceContext);

/*
* Called every frame within renderOverlay() for the ImGui components displayed within the menu.
*/
void __stdcall renderContent();

/*
* Called every frame to render the ImGui overlay from inside our hook.
*/
void __stdcall renderOverlay(ID3D11Device* device, ID3D11DeviceContext* deviceContext);

/*
* Style the ImGui overlay. 
* credits to https://github.com/adobe/imgui/blob/master/imgui_spectrum.h
*/
void __stdcall setImGuiStyle();

void __stdcall showMemoryTable();

