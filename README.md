# Minimal SDL2.dll proxy for FFVII.exe + sdl2-compat 2.xx.xx

## Why 

FFVII.exe never pumps SDL events and never calls SDL_NumJoysticks().
sdl2-compat only rebuilds its cached "device index -> SDL3 joystick ID"
list inside SDL_NumJoysticks() (and when it converts SDL3 device-added
events, and once at SDL_Init). So after startup, SDL_IsGameController(i)
and SDL_GameControllerOpen(i) keep using a stale list and never see a
controller plugged in later. Original SDL2 reads its live device list.

## Fix

Before IsGameController/Open, call the real SDL_NumJoysticks() so the
compat list is current. Everything else is forwarded unchanged.

Only the 11 SDL functions FFVII.exe imports are exported.

## Setup in the game folder:
```
FINAL FANTASY VII Steam/GOG Edition
 |
 |--- SDL2.dll         <- this proxy (built from this file)
 |--- SDL2_compat.dll  <- the sdl2-compat v2.xx.xx SDL2.dll, renamed
 |--- SDL3.dll         <- unchanged
```

 ## Build (x64):

### MSVC:   
```
cl /LD /O2 sdl2_proxy.c /Fe:SDL2.dll
```

### MinGW:  
```
x86_64-w64-mingw32-gcc -shared -O2 -o SDL2.dll sdl2_proxy.c
```
