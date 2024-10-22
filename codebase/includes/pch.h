#pragma once

#ifndef IMGUI_DEFINE_MATH_OPERATORS
	#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include <Windows.h>
#include <iostream>
#include <string>
#include <d3d9.h>
#include <d3dx9.h>
#include <cstdio>
#include <tchar.h>
#include <vector>
#include <map>
#include <algorithm>
#include <functional>
#include <memory>
#include <cstdint>
#include <sstream>
#include <variant>
#include <cmath>
#include <math.h>
#include <TlHelp32.h>

// link dx9 stuff
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

// imgui stuff
#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include "imgui_style.h"
#include "imgui_internal.h"
#include "misc/cpp/imgui_stdlib.h"

#include "logger.h"

