#pragma once

#include "pch.h"

extern bool initialized; // flag to check if ImGui is initialized
extern ImGuiIO* io; // stored globally to reduce overhead inside renderOverlay

/*
* Initializes ImGui overlay inside our hook.
* @return S_OK if successful, E_FAIL otherwise
*/
HRESULT __stdcall initOverlay();

/*
* Called every frame within renderOverlay() for the ImGui components displayed within the menu.
*/
void __stdcall renderContent();


/*
* Called every frame to render the ImGui overlay from inside our hook.
*/
void __stdcall renderOverlay();

