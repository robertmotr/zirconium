# zirconium
DLL-injectable game cheat for Call of Duty: Black Ops 2 Zombies (with the Plutonium mod).
- Downloads are in the [Releases](https://www.github.com/robertmotr/zirconium/releases) page

# Disclaimer:
- This project exists because I grew up on BO2, and it also happened to be a good target for reversing. It's a 14 year old AAA game thats practically dead, but also happens to be a somewhat modern application for trying to reverse a big game engine.
- I do not condone cheating. I am not responsible for when you inevitably get banned by using this. Thus, you should use this at your own risk.

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
git clone --recurse-submodules https://www.github.com/robertmotr/zirconium.git
cd zirconium/codebase
cmake -B ../cmake-build -G "Visual Studio 17 2022" -A Win32
cmake --build ../cmake-build --config Release
```
DLL will be at `../build/Release/zirconium.dll`

# Usage:
1. Open the game, load into a zombies match
2. Use [Xenos injector](https://github.com/DarthTon/Xenos) to find `plutonium-bootstrapper-win32.exe` in the process list and select it 
4. Load `zirconium.dll` as the DLL and inject
5. I have only used and tested the default native inject options, I have no idea if anything else like manual mapping would work. The same idea goes for any of the 1 billion different implementations of DLL injectors out there.

# Credits:
Thank you to all the people on unknowncheats.me who have reversed tons of offsets :) 

# Interesting observations I came across:
- There exists a PDB file of the multiplayer server, which shares a lot of common functions with the client (even with Plutonium modifications). This can help speed up reversing by cross checking your progress in IDA against the PDB, but only to an extent. 
- Plutonium uses Themida, but only certain parts of the .text section are obfuscated. Getting past the anti-debug just requires you to use ScyllaHide's Themida profile on x64dbg. It doesn't seem to use all the crazy Themida features to make reversing hell.
- Plutonium runs a checksum on the .text section (maybe more sections) that verifies no bytes were changed against the original code. This means that any modification whatsoever in the .text section will change the checksum. The best way to hook functions is to use hardware breakpoints (https://en.wikipedia.org/wiki/X86_debug_register) and have them jump into vectored exception handlers for the functions you want to hook into. This will keep the checksum intact and thus undetected.
  - Considering that DX11 is a DLL outside of the .text section it might be possible that it is not necessary to use hardware breakpoints + VEH to hook into `IDXGISwapChain::Present` without getting detected. Originally I spent a year of on and off progress trying to figure out how to trampoline hook this function (which you can still use if you toggle a build flag in `CMakeLists.txt`), but after discovering the checksum I ditched it and went with hooking with hardware breakpoints for everything.       

## TODO:
- i asked claude to clean up my code and it just turned it into slop... maybe one day unslopify back into my original code
- aimbot smoothing/more aimbot options
- spinbot
- silent aim
- engine chams
- magic wand/zombie pile(?)

## Done:
- upload executable in Releases
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
