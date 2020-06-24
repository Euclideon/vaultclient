# To execute this in WSL from a Windows checkout, run the following to convert to Unix line endings:
#    cat buildscripts/apply-emscripten-mods.sh | sed 's/\r//' | bash

echo "Apply modifications to Emscripten"

export EMSCRIPTEN_SOURCE=${EMSDK}/upstream/emscripten
export EMSCRIPTEN_MODS=emscripten_mods

if [[ ! -d "${EMSCRIPTEN_MODS}" ]]; then
	export EMSCRIPTEN_MODS=buildscripts/${EMSCRIPTEN_MODS}
	if [[ ! -d "${EMSCRIPTEN_MODS}" ]]; then
		echo "Could not locate emscripten_mods folder, run from git root or buildscripts folder!"
		exit 1;
	fi
fi

# Detect SDL2 version
if grep -q "TAG = 'version_18'" "${EMSCRIPTEN_SOURCE}/tools/ports/sdl2.py"; then
	export EMSCRIPTEN_SDL2_PORT=${HOME}/.emscripten_ports/sdl2/SDL2-version_18/src
	export EMSCRIPTEN_SDL_MODS=${EMSCRIPTEN_MODS}/SDL2/version_18
elif grep -q "TAG = 'version_21'" "${EMSCRIPTEN_SOURCE}/tools/ports/sdl2.py"; then
	export EMSCRIPTEN_SDL2_PORT=${EMSCRIPTEN_SOURCE}/cache/ports/sdl2/SDL2-version_21/src
	export EMSCRIPTEN_SDL_MODS=${EMSCRIPTEN_MODS}/SDL2/version_21
else
	echo "Unsupported version of SDL2 detected!"
	exit 1
fi

if [[ ! -f "${EMSCRIPTEN_SOURCE}/src/library_egl_bak.js" ]]; then
	# This isn't the correct solution
	echo "Modify library_egl.js so that it doesn't proxy to the main thread for invalid functions"
	
	# Backup original file first
	cp ${EMSCRIPTEN_SOURCE}/src/library_egl.js ${EMSCRIPTEN_SOURCE}/src/library_egl_bak.js
	
	sed -i "s/  eglCreateContext__proxy: 'sync',/#if \!OFFSCREENCANVAS_SUPPORT\n  eglCreateContext__proxy: 'sync',\n#endif/" ${EMSCRIPTEN_SOURCE}/src/library_egl.js
	sed -i "s/  eglMakeCurrent__proxy: 'sync',/#if \!OFFSCREENCANVAS_SUPPORT\n  eglMakeCurrent__proxy: 'sync',\n#endif/" ${EMSCRIPTEN_SOURCE}/src/library_egl.js
fi

if [[ ! -f "${EMSCRIPTEN_SOURCE}/src/library_html5_bak.js" ]]; then
	echo "Apply a patch from a newer version of Emscripten, also don't run canvasResizedCallback on target thread"
	
	# Backup original file first
	cp ${EMSCRIPTEN_SOURCE}/src/library_html5.js ${EMSCRIPTEN_SOURCE}/src/library_html5_bak.js
	
	# Apply a patch from a newer version of Emscripten, not required after 1.39.3
	sed -i "s/^    if (__currentFullscreenStrategy.canvasResizedCallback)/    if (false)/" ${EMSCRIPTEN_SOURCE}/src/library_html5.js
	
	# Don't run canvasResizedCallback on target thread
	sed -i "s/^          if (__currentFullscreenStrategy.canvasResizedCallbackTargetThread)/          if (false)/"  ${EMSCRIPTEN_SOURCE}/src/library_html5.js
fi

if [[ ! -f "${EMSCRIPTEN_SOURCE}/tools/ports/sdl2_bak.py" ]]; then
	echo "Add clipboard source and modify sdl2.py so that it compiles it"
	
	# Backup original file first
	cp ${EMSCRIPTEN_SOURCE}/tools/ports/sdl2.py ${EMSCRIPTEN_SOURCE}/tools/ports/sdl2_bak.py
	
	# Copy clipboard source in
	cp ${EMSCRIPTEN_SDL_MODS}/SDL_emscriptenclipboard.* ${EMSCRIPTEN_SDL2_PORT}/video/emscripten
	
	# Modify sdl2.py to compile clipboard source
	sed -i 's%video/emscripten/SDL_emscriptenevents.c%video/emscripten/SDL_emscriptenclipboard.c video/emscripten/SDL_emscriptenevents.c%' ${EMSCRIPTEN_SOURCE}/tools/ports/sdl2.py
fi

if [ ! -f "${EMSCRIPTEN_SDL2_PORT}/video/emscripten/SDL_emscriptenframebuffer_bak.c" ]; then
	echo "Update framebuffer, mouse and video source to work with PROXY_TO_PTHREAD and use clipboard API"
	
	# Backup original files first
	cp ${EMSCRIPTEN_SDL2_PORT}/video/emscripten/SDL_emscriptenframebuffer.c ${EMSCRIPTEN_SDL2_PORT}/video/emscripten/SDL_emscriptenframebuffer_bak.c
	cp ${EMSCRIPTEN_SDL2_PORT}/video/emscripten/SDL_emscriptenmouse.c ${EMSCRIPTEN_SDL2_PORT}/video/emscripten/SDL_emscriptenmouse_bak.c
	cp ${EMSCRIPTEN_SDL2_PORT}/video/emscripten/SDL_emscriptenvideo.c ${EMSCRIPTEN_SDL2_PORT}/video/emscripten/SDL_emscriptenvideo_bak.c
	
	# Replace files with modified versions
	cp ${EMSCRIPTEN_SDL_MODS}/SDL_emscriptenframebuffer.c ${EMSCRIPTEN_SDL2_PORT}/video/emscripten/SDL_emscriptenframebuffer.c
	cp ${EMSCRIPTEN_SDL_MODS}/SDL_emscriptenmouse.c ${EMSCRIPTEN_SDL2_PORT}/video/emscripten/SDL_emscriptenmouse.c
	cp ${EMSCRIPTEN_SDL_MODS}/SDL_emscriptenvideo.c ${EMSCRIPTEN_SDL2_PORT}/video/emscripten/SDL_emscriptenvideo.c
fi

if [ ! -f "${EMSCRIPTEN_SDL2_PORT}/filesystem/emscripten/SDL_sysfilesystem_bak.c" ]; then
	echo "Update filesystem source to create intermediate directories"
	
	# Backup original file first
	cp ${EMSCRIPTEN_SDL2_PORT}/filesystem/emscripten/SDL_sysfilesystem.c ${EMSCRIPTEN_SDL2_PORT}/filesystem/emscripten/SDL_sysfilesystem_bak.c
	
	# Replace file with modified version
	cp ${EMSCRIPTEN_SDL_MODS}/SDL_sysfilesystem.c ${EMSCRIPTEN_SDL2_PORT}/filesystem/emscripten/SDL_sysfilesystem.c
fi
