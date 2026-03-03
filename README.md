# zirconium
DLL-injectable game cheat for Plutonium Black Ops 2 Zombies based off reverse-engineered memory offsets and game code

# LAN ONLY!!! I have never tested this with the anticheat on, and considering my code you'd probably get banned 

# Building:
1. `git clone https://www.github.com/robertmotr/zirconium.git`
2. `cd codebase`
3. `cmake -B ../cmake-build -G "Visual Studio 17 2022" -A Win32`
4. `cmake --build ../cmake-build`
5. DLL will be located in `..\build\Debug\zirconium.dll`

# Usage:
1. Open the game, load into a zombies match
2. Use [Xenos injector](https://github.com/DarthTon/Xenos) to find `plutonium-bootstrapper-win32.exe` in the process list and select it 
4. Load `zirconium.dll` as the DLL and inject
5. I have only used and tested the default native inject options, I have no idea if anything else like manual mapping would work. The same idea goes for any of the 1 billion DLL injectors out there

# Credits:
Thank you to all the people on unknowncheats.me who have reversed tons of offsets :) 

## TODO:
- upload executable in Releases
- ESP
  - ~~box~~ (done)
  - skeleton
  - snap lines
  - distance
  - ~~health~~ (done)
- aimbot
- perks
- teleport

## Done:
- refactor logging
  - coloured text based on log level
  - remove weird recursive template solution
  - add log level + conditional logging based on level
- port build system to cmake from visual studio
- uninject DLL option and reset state
- better debug tab
