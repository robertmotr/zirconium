#pragma once

#include "pch.h"

namespace zirconium {

    /*
    * Style the ImGui overlay.
    * credits to https://github.com/adobe/imgui/blob/master/imgui_spectrum.h
    */
    void __stdcall overlay_setImGuiStyle();

    /*
    * Called every frame within render_frame() for the ImGui components displayed within the menu.
    */
    void __stdcall overlay_show();

}
