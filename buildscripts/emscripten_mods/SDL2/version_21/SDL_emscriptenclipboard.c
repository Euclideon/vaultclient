/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2018 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../SDL_internal.h"

#if SDL_VIDEO_DRIVER_EMSCRIPTEN

#include "SDL_emscriptenvideo.h"
#include "../../events/SDL_clipboardevents_c.h"

#include <emscripten/threading.h>

void
Emscripten_SetClipboardTextWorker(const char *text, unsigned int *waitValue)
{
    EM_ASM_({
        if (typeof(navigator.clipboard) !== "undefined") {
            navigator.clipboard.writeText(UTF8ToString($0)).then(
                function () { // Success
                    Atomics.store(HEAPU32, $1 >> 2, 0);
                    _emscripten_futex_wake($1, 1);
                },
                function () { // Failure
                    Atomics.store(HEAPU32, $1 >> 2, 1);
                    _emscripten_futex_wake($1, 1);
                }
            );
        } else {
            Atomics.store(HEAPU32, $1 >> 2, 1);
            _emscripten_futex_wake($1, 1);
        }
    }, text, waitValue);
}

int
Emscripten_SetClipboardText(_THIS, const char *text)
{
    unsigned int waitValue;

    waitValue = 0xFFFFFFFF;
    if (emscripten_is_main_runtime_thread()) {
        Emscripten_SetClipboardTextWorker(text, &waitValue);
    } else {
        emscripten_sync_run_in_main_runtime_thread(
            EM_FUNC_SIG_VII,
            Emscripten_SetClipboardTextWorker,
            (uint32_t)text,
            (uint32_t)&waitValue
        );
    }
    emscripten_futex_wait(&waitValue, 0xFFFFFFFF, INFINITY);

    return waitValue;
}

void
Emscripten_GetClipboardTextWorker(char **text, unsigned int *waitValue)
{
    EM_ASM_({
        if (typeof(navigator.clipboard) !== "undefined") {
            navigator.clipboard.readText().then(
                function (clipText) {
                    var lengthBytes = lengthBytesUTF8(clipText) + 1;
                    var allocText = _malloc(lengthBytes);
                    stringToUTF8(clipText, allocText, lengthBytes + 1);
                    HEAPU32[$0 >> 2] = allocText;
                    Atomics.store(HEAPU32, $1 >> 2, 0);
                    _emscripten_futex_wake($1, 1);
                }
            );
        } else {
            HEAPU32[$0 >> 2] = 0;
            Atomics.store(HEAPU32, $1 >> 2, 0);
            _emscripten_futex_wake($1, 1);
        }
    }, text, waitValue);
}

char *
Emscripten_GetClipboardText(_THIS)
{
    char *text;
    unsigned int waitValue;

    text = NULL;
    waitValue = 1;
    if (emscripten_is_main_runtime_thread()) {
        Emscripten_GetClipboardTextWorker(&text, &waitValue);
    } else {
        emscripten_sync_run_in_main_runtime_thread(
            EM_FUNC_SIG_VII,
            Emscripten_GetClipboardTextWorker,
            (uint32_t)&text,
            (uint32_t)&waitValue
        );
    }
    emscripten_futex_wait(&waitValue, 1, INFINITY);
    return text;
}

#endif /* SDL_VIDEO_DRIVER_EMSCRIPTEN */

/* vi: set ts=4 sw=4 expandtab: */
