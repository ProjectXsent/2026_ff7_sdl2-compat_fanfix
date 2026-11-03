/*
 * Minimal SDL2.dll proxy for FFVII.exe + sdl2-compat 2.xx.xx
 *
 * Why: FFVII.exe never pumps SDL events and never calls SDL_NumJoysticks().
 * sdl2-compat only rebuilds its cached "device index -> SDL3 joystick ID"
 * list inside SDL_NumJoysticks() (and when it converts SDL3 device-added
 * events, and once at SDL_Init). So after startup, SDL_IsGameController(i)
 * and SDL_GameControllerOpen(i) keep using a stale list and never see a
 * controller plugged in later. Original SDL2 reads its live device list.
 *
 * Fix: before IsGameController/Open, call the real SDL_NumJoysticks() so the
 * compat list is current. Everything else is forwarded unchanged.
 *
 * Only the 11 SDL functions FFVII.exe imports are exported.
 *
 * Setup in the game folder:
 *   SDL2.dll         <- this proxy (built from this file)
 *   SDL2_compat.dll  <- the sdl2-compat v2.xx.xx SDL2.dll, renamed
 *   SDL3.dll         <- unchanged
 *
 * Build (x64):
 *   MSVC:   cl /LD /O2 sdl2_proxy.c /Fe:SDL2.dll
 *   MinGW:  x86_64-w64-mingw32-gcc -shared -O2 -o SDL2.dll sdl2_proxy.c
 */
#include <windows.h>

#define EXPORT __declspec(dllexport)

static HMODULE g_mod;

static FARPROC real(const char *name)
{
    if (!g_mod) {
        g_mod = LoadLibraryA("SDL2_compat.dll");
        if (!g_mod) return NULL;
    }
    return GetProcAddress(g_mod, name);
}

static void refresh_device_list(void)
{
    static int (__cdecl *num_joysticks)(void);
    if (!num_joysticks) num_joysticks = (int (__cdecl *)(void))real("SDL_NumJoysticks");
    if (num_joysticks) num_joysticks();
}

/* Load gamecontrollerdb.txt from the folder FFVII.exe lives in (once). */
static void load_controller_db(void)
{
    static int done;
    char path[MAX_PATH + 32];
    DWORD n, i, last = 0;
    const char *file = "gamecontrollerdb.txt";
    void *(__cdecl *rw_from_file)(const char *, const char *);
    int (__cdecl *add_from_rw)(void *, int);
    void *rw;

    if (done) return;
    done = 1;

    n = GetModuleFileNameA(NULL, path, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return;
    for (i = 0; i < n; i++)
        if (path[i] == '\\' || path[i] == '/') last = i + 1;
    for (i = 0; file[i]; i++) path[last + i] = file[i];
    path[last + i] = '\0';

    if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES) return;

    rw_from_file = (void *(__cdecl *)(const char *, const char *))real("SDL_RWFromFile");
    add_from_rw = (int (__cdecl *)(void *, int))real("SDL_GameControllerAddMappingsFromRW");
    if (!rw_from_file || !add_from_rw) return;

    rw = rw_from_file(path, "rb");
    if (rw) add_from_rw(rw, 1);   /* 1 = SDL closes the file */
}

EXPORT int SDL_Init(unsigned int flags)
{
    static int (__cdecl *fn)(unsigned int);
    int rc;
    if (!fn) fn = (int (__cdecl *)(unsigned int))real("SDL_Init");
    rc = fn ? fn(flags) : -1;
    if (rc == 0) load_controller_db();
    return rc;
}

EXPORT int SDL_SetHint(const char *name, const char *value)
{
    static int (__cdecl *fn)(const char *, const char *);
    if (!fn) fn = (int (__cdecl *)(const char *, const char *))real("SDL_SetHint");
    return fn ? fn(name, value) : 0;
}

EXPORT int SDL_JoystickEventState(int state)
{
    static int (__cdecl *fn)(int);
    if (!fn) fn = (int (__cdecl *)(int))real("SDL_JoystickEventState");
    return fn ? fn(state) : 0;
}

EXPORT int SDL_IsGameController(int index)
{
    static int (__cdecl *fn)(int);
    if (!fn) fn = (int (__cdecl *)(int))real("SDL_IsGameController");
    refresh_device_list();
    return fn ? fn(index) : 0;
}

EXPORT void *SDL_GameControllerOpen(int index)
{
    static void *(__cdecl *fn)(int);
    if (!fn) fn = (void *(__cdecl *)(int))real("SDL_GameControllerOpen");
    refresh_device_list();
    return fn ? fn(index) : NULL;
}

EXPORT void SDL_GameControllerClose(void *gc)
{
    static void (__cdecl *fn)(void *);
    if (!fn) fn = (void (__cdecl *)(void *))real("SDL_GameControllerClose");
    if (fn) fn(gc);
}

EXPORT void SDL_GameControllerUpdate(void)
{
    static void (__cdecl *fn)(void);
    if (!fn) fn = (void (__cdecl *)(void))real("SDL_GameControllerUpdate");
    if (fn) fn();
}

EXPORT int SDL_GameControllerGetAttached(void *gc)
{
    static int (__cdecl *fn)(void *);
    if (!fn) fn = (int (__cdecl *)(void *))real("SDL_GameControllerGetAttached");
    return fn ? fn(gc) : 0;
}

EXPORT int SDL_GameControllerGetType(void *gc)
{
    static int (__cdecl *fn)(void *);
    if (!fn) fn = (int (__cdecl *)(void *))real("SDL_GameControllerGetType");
    return fn ? fn(gc) : 0;
}

EXPORT short SDL_GameControllerGetAxis(void *gc, int axis)
{
    static short (__cdecl *fn)(void *, int);
    if (!fn) fn = (short (__cdecl *)(void *, int))real("SDL_GameControllerGetAxis");
    return fn ? fn(gc, axis) : 0;
}

EXPORT unsigned char SDL_GameControllerGetButton(void *gc, int button)
{
    static unsigned char (__cdecl *fn)(void *, int);
    if (!fn) fn = (unsigned char (__cdecl *)(void *, int))real("SDL_GameControllerGetButton");
    return fn ? fn(gc, button) : 0;
}
