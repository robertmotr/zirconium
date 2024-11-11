#pragma once

#define _CRT_SECURE_NO_WARNINGS

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include <Windows.h>
#include <iostream>
#include <string>
#include <dxgi.h>
#include <d3d11.h>
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
#include <dbghelp.h>
#include <fstream>
#include <cstdlib>

// link dx9 stuff
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dbghelp.lib")

// imgui stuff
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "imgui_style.h"
#include "imgui_internal.h"
#include "misc/cpp/imgui_stdlib.h"

#include "logger.h"
