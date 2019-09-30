set -xeu

git clean -ffxd
git submodule foreach --recursive 'git clean -ffdx'

git submodule sync
git submodule update --init
git submodule foreach --recursive "git submodule sync && git submodule update --init"

if [ $OSTYPE == "msys" ]; then # Windows, MinGW
	export DEV="//bne-fs-fs-003.euclideon.local/Dev"
else
	export DEV="/mnt/Dev"
fi

export DEPLOYDIR="$DEV/Builds/vault/client/Pipeline_$CI_PIPELINE_ID"
export VAULTSDK_HOME="$DEV/Builds/vault/linkedvdk/Pipeline_44784"

# Prepare UserGuide
mkdir -p builds/userguide
cp -r docs/images builds/userguide/images
cp docs/UserGuide.md builds/userguide/UserGuide.md

if [ $OSTYPE == "msys" ]; then # Windows, MinGW
	export CERT_THUMBPRINT="cfa975b624d31714cce95753ffcdee9465b6f073"

	3rdParty/udcore/bin/premake-bin/premake5.exe vs2015 $3

	"C:/Program Files (x86)/MSBuild/14.0/Bin/amd64/MSBuild.exe" vaultClient.sln //p:Configuration=$1 //p:Platform=$2 //v:m //m

	# Sign binaries
	if [ $3 == "--gfxapi=d3d11" ]; then
		"C:/Program Files (x86)/Windows Kits/8.1/bin/x64/signtool.exe" sign //v //d "Euclideon Vault Client" //sha1 $CERT_THUMBPRINT //t http://timestamp.digicert.com builds\\vaultClient_d3d11.exe
	else
		"C:/Program Files (x86)/Windows Kits/8.1/bin/x64/signtool.exe" sign //v //d "Euclideon Vault Client" //sha1 $CERT_THUMBPRINT //t http://timestamp.digicert.com builds\\vaultClient.exe
		"C:/Program Files (x86)/Windows Kits/8.1/bin/x64/signtool.exe" sign //v //d "Euclideon Vault Convert CMD" //sha1 $CERT_THUMBPRINT //t http://timestamp.digicert.com builds\\vaultConvertCMD.exe
	fi

	if [ $1 == "Release" ] && ([ $CI_BUILD_REF_NAME == "master" ] || [ -n "$CI_BUILD_TAG" ]); then
		# Make sure directory exists
		mkdir -p $DEPLOYDIR/Windows

		//bne-fs-fs-003.euclideon.local/Software/DevelopmentTools/Pandoc/pandoc-2.3-windows-x86_64/pandoc.exe -f gfm --tab-stop 2 docs/UserGuide.md -o __temp.html
		cat docs/misc/header.html __temp.html docs/misc/footer.html > builds/userguide/UserGuide.html
		rm __temp.html

		//bne-fs-fs-003.euclideon.local/Software/DevelopmentTools/Pandoc/pandoc-2.3-windows-x86_64/pandoc.exe -f gfm --tab-stop 2 docs/TranslationGuide.md -o __temp.html
		cat docs/misc/header.html __temp.html docs/misc/footer.html > builds/userguide/TranslationGuide.html
		rm __temp.html

		# D3D copies only EXE, OpenGL copies everything else
		if [ $3 == "--gfxapi=d3d11" ]; then
			cp -f builds/vaultClient_d3d11.exe $DEPLOYDIR/Windows/vaultClient.exe
		else
			cp -f builds/SDL2.dll $DEPLOYDIR/Windows/SDL2.dll
			cp -f builds/vaultClient.exe $DEPLOYDIR/Windows/vaultClient_OpenGL.exe
			cp -f builds/vaultConvertCMD.exe $DEPLOYDIR/Windows/vaultConvertCMD.exe
			cp -rf builds/assets/ $DEPLOYDIR/Windows/assets
			cp -f $VAULTSDK_HOME/lib/win_x64/vaultSDK.dll $DEPLOYDIR/Windows/vaultSDK.dll
			cp -f builds/releasenotes.md $DEPLOYDIR/Windows/releasenotes.md
			cp -f builds/defaultsettings.json $DEPLOYDIR/Windows/defaultsettings.json
			cp -rf builds/userguide/ $DEPLOYDIR/Windows/userguide
      
			if [ -f "builds/libfbxsdk.dll" ]; then
				cp -f builds/libfbxsdk.dll $DEPLOYDIR/Windows/libfbxsdk.dll
			fi

			# Technically this could be in any build; outputs the change list to the deploy dir
			git log --no-merges --pretty=format:"%an (%ae) %ai%n%s%n%b" > $DEPLOYDIR/history.txt
		fi
	fi
else
	if ([[ $OSTYPE == "darwin"* ]] && [ $3 == "ios" ]); then # iOS
		export OSNAME="iOS"
		3rdParty/udcore/bin/premake-bin/premake5-osx xcode4 --os=ios
		if [ $2 == "x86_64" ]; then # Simulator
			xcodebuild -project vaultClient.xcodeproj -configuration $1 -arch $2 -sdk iphonesimulator
		else
			xcodebuild -project vaultClient.xcodeproj -configuration $1 -arch $2
		fi
	elif [[ $OSTYPE == "darwin"* ]]; then # OSX
		export OSNAME="macOS"
		3rdParty/udcore/bin/premake-bin/premake5-osx xcode4 $3
		xcodebuild -project vaultClient.xcodeproj -configuration $1
	elif ([ -n "$3" ] && [ $3 == "Emscripten" ]); then # Emscripten
		export OSNAME="Emscripten"

		pushd /emsdk
		source ./emsdk_env.sh
		popd

		# Apply modifications to Emscripten
		# Modify library_egl.js so that it doesn't proxy to the main thread for invalid functions
		sed -i "s/  eglCreateContext__proxy: 'sync',/#if \!OFFSCREENCANVAS_SUPPORT\n  eglCreateContext__proxy: 'sync',\n#endif/" /emsdk/fastcomp/emscripten/src/library_egl.js
		sed -i "s/  eglMakeCurrent__proxy: 'sync',/#if \!OFFSCREENCANVAS_SUPPORT\n  eglMakeCurrent__proxy: 'sync',\n#endif/" /emsdk/fastcomp/emscripten/src/library_egl.js
		# Add clipboard source and modify sdl2.py so that it compiles it
		cp buildscripts/emscripten_mods/SDL2/SDL_emscriptenclipboard.* /root/.emscripten_ports/sdl2/SDL2-version_18/src/video/emscripten
		sed -i 's%video/emscripten/SDL_emscriptenevents.c%video/emscripten/SDL_emscriptenclipboard.c video/emscripten/SDL_emscriptenevents.c%' /emsdk/fastcomp/emscripten/tools/ports/sdl2.py
		# Update framebuffer, mouse and video source to work with PROXY_TO_PTHREAD and use clipboard API
		cp buildscripts/emscripten_mods/SDL2/SDL_emscriptenframebuffer.c /root/.emscripten_ports/sdl2/SDL2-version_18/src/video/emscripten/SDL_emscriptenframebuffer.c
		cp buildscripts/emscripten_mods/SDL2/SDL_emscriptenmouse.c /root/.emscripten_ports/sdl2/SDL2-version_18/src/video/emscripten/SDL_emscriptenmouse.c
		cp buildscripts/emscripten_mods/SDL2/SDL_emscriptenvideo.c /root/.emscripten_ports/sdl2/SDL2-version_18/src/video/emscripten/SDL_emscriptenvideo.c
		# Update filesystem source to create intermediate directories
		cp buildscripts/emscripten_mods/SDL2/SDL_sysfilesystem.c /root/.emscripten_ports/sdl2/SDL2-version_18/src/filesystem/emscripten/SDL_sysfilesystem.c

		3rdParty/udcore/bin/premake-bin/premake5 gmake2 --os=emscripten

		# TODO: Use ${2} instead of ${3} like all other targets
		make config=$(echo ${1}_${3} | tr [:upper:] [:lower:]) -j4

		# Copy HTML page into builds folder ready for packaging
		cp buildscripts/vaultClient.html builds/vaultClient.html
	else
		export OSNAME="Linux"
		3rdParty/udcore/bin/premake-bin/premake5 gmake
		make config=$(echo ${1}_${2} | tr [:upper:] [:lower:]) -j4
	fi

	if ([[ $OSTYPE == "darwin"* ]] && [ $3 == "ios" ]); then # iOS
		# Make folder to store the framework to build a DMG from
		mkdir builds/packaging
		cp -af builds/vaultClient.app builds/packaging/vaultClient.app
		export APPNAME=vaultClient
		hdiutil create builds/vaultClient.dmg -volname "vaultClient" -srcfolder builds/packaging
	elif [[ $OSTYPE == "darwin"* ]]; then # OSX
		# Make folder to store the framework to build a DMG from
		mkdir -p builds/packaging/.background
		if [ $3 == "--gfxapi=metal" ]; then
			cp -af builds/vaultClient_metal.app builds/packaging/vaultClient_metal.app
			export APPNAME=vaultClient_metal
		else
			cp -af builds/vaultClient.app builds/packaging/vaultClient.app
			export APPNAME=vaultClient
		fi
		cp icons/dmgBackground.png builds/packaging/.background/background.png

		# Detach /Volumes/vaultClient just in case an earlier build failed to do so
		# || true as this command is allowed to fail
		hdiutil detach /Volumes/vaultClient || true

		# See https://stackoverflow.com/a/1513578 for reference
		hdiutil create builds/vaultClient.temp.dmg -volname "vaultClient" -srcfolder builds/packaging -format UDRW -fs HFS+ -fsargs "-c c=64,a=16,e=16"
		device=$(hdiutil attach -readwrite -noverify -noautoopen "builds/vaultClient.temp.dmg" | egrep '^/dev/' | sed 1q | awk '{print $1}')
		# Delay 10 to give the disk time to mount - otherwise builds fail
		echo '
			delay 10
			tell application "Finder"
				tell disk "vaultClient"
					open
					set current view of container window to icon view
					set toolbar visible of container window to false
					set statusbar visible of container window to false
					set the bounds of container window to {400, 100, (400 + 512), (100 + 320 + 23)}
					set theViewOptions to the icon view options of container window
					set arrangement of theViewOptions to not arranged
					set icon size of theViewOptions to 72
					set background picture of theViewOptions to file ".background:background.png"
					make new alias file at container window to POSIX file "/Applications" with properties {name:"Applications"}
					set position of item "APPNAME" of container window to {150, 180}
					set position of item "Applications" of container window to {360, 180}
					update without registering applications
					delay 5
					close
				end tell
			end tell
		' | sed "s/APPNAME/$APPNAME/" | osascript
		chmod -Rf go-w /Volumes/vaultClient
		cp icons/macOSAppIcons.icns /Volumes/vaultClient/.VolumeIcon.icns
		SetFile -c icnC "/Volumes/vaultClient/.VolumeIcon.icns"
		SetFile -a C /Volumes/vaultClient
		sync
		sync
		hdiutil detach ${device}
		hdiutil convert "builds/vaultClient.temp.dmg" -format UDZO -imagekey zlib-level=9 -o "builds/vaultClient.dmg"
		rm -f builds/vaultClient.temp.dmg
		rm -rf builds/packaging
	else
		pushd builds
		tar -zcvf ../vaultClient-$OSNAME.tar.gz *
		popd
	fi

	if ([ $CI_BUILD_REF_NAME == "master" ] || [ -n "$CI_BUILD_TAG" ]); then
		# We build for both GCC and Clang, so need to handle them seperately
		if [ $1 == "Release" ] ; then
			# Make sure directory exists
			mkdir -p $DEPLOYDIR

			if [[ $OSTYPE == "darwin"* ]]; then # iOS or macOS
				if ([ $2 == "arm64" ] || [ $2 == "x64" ]); then # Don't deploy simulator builds of iOS
					cp -f builds/vaultClient.dmg $DEPLOYDIR/$APPNAME-$OSNAME.dmg
				fi
			else
				cp -f vaultClient-$OSNAME.tar.gz $DEPLOYDIR/vaultClient-$OSNAME.tar.gz
			fi
		fi
	fi
fi
