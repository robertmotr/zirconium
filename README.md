# zirconium
DLL-injectable game cheat for Plutonium Black Ops 2 Zombies. Currently undetected in Plutonium.
- Downloads are in the [Releases](https://www.github.com/robertmotr/zirconium/releases) page

## Preview:
![Aimbot](aimbot_preview.gif)

![Misc features](misc_preview.gif)

# Features:
- God mode
- Invisibility from zombies
- ESP
- Aimbot
- Third person FOV
- No spread
- No recoil
- Teleport
- Set the following:
  - Money
  - Ammo
  - Grenades/claymores/monkey bombs etc
  - Jump height
  - Gravity
  - Speed

# Building:
```
git clone --recurse-submodules https://www.github.com/robertmotr/zirconium.git`
cd zirconium/codebase
cmake -B ../cmake-build -G "Visual Studio 17 2022" -A Win32
cmake --build ../cmake-build --config Release
```
DLL will be at ..\build\Release\zirconium.dll

# Usage:
1. Open the game, load into a zombies match
2. Use [Xenos injector](https://github.com/DarthTon/Xenos) to find `plutonium-bootstrapper-win32.exe` in the process list and select it 
4. Load `zirconium.dll` as the DLL and inject
5. I have only used and tested the default native inject options, I have no idea if anything else like manual mapping would work. The same idea goes for any of the 1 billion DLL injectors out there

# Credits:
Thank you to all the people on unknowncheats.me who have reversed tons of offsets :) 

## TODO:
- upload executable in Releases
- aimbot smoothing
- spinbot
- magic wand/zombie pile(?)

## Done:
- third person
- refactor logging
  - coloured text based on log level
  - remove weird recursive template solution
  - add log level + conditional logging based on level
- port build system to cmake from vs
- aimbot
- uninject DLL option and reset state
- better debug tab
- teleport
- god mode
- ESP
  - skeleton
  - snap lines
  - distance
  - health
