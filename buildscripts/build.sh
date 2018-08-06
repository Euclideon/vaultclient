git clean -ffxd
git submodule foreach --recursive 'git clean -ffdx'

git submodule sync --recursive
git submodule update --init --recursive
if [ $? -ne 0 ]; then exit 1; fi

if [ $OSTYPE == "msys" ]; then # Windows, MinGW
	export RESOURCES="//bne-fs-fs-003.euclideon.local/Resources"
else
	export RESOURCES="/mnt/Resources"
fi

export DEPLOYDIR="$RESOURCES/Builds/vault/client/Pipeline_$CI_PIPELINE_ID"
export VAULTSDK_HOME="$RESOURCES/Builds/vault/sdk/Pipeline_27083"

if [ $OSTYPE == "msys" ]; then # Windows, MinGW
	export CERT_THUMBPRINT="bbee8d60b45735badf16f76e049b966981bd2751"

	bin/premake/premake5.exe vs2015 $3
	if [ $? -ne 0 ]; then exit 1; fi

	"C:/Program Files (x86)/MSBuild/14.0/Bin/amd64/MSBuild.exe" vaultClient.sln //p:Configuration=$1 //p:Platform=$2 //v:m //m
	if [ $? -ne 0 ]; then exit 1; fi

	# Sign binaries
	if [ $3 == "--gfxapi=d3d11" ]; then
		"C:/Program Files (x86)/Windows Kits/8.1/bin/x64/signtool.exe" sign //v //d "Euclideon Client" //sha1 $CERT_THUMBPRINT //t http://timestamp.digicert.com builds\\vaultClient_d3d11.exe
		if [ $? -ne 0 ]; then exit 1; fi
	else
		"C:/Program Files (x86)/Windows Kits/8.1/bin/x64/signtool.exe" sign //v //d "Euclideon Client" //sha1 $CERT_THUMBPRINT //t http://timestamp.digicert.com builds\\vaultClient.exe
		if [ $? -ne 0 ]; then exit 1; fi
	fi

	if [ $1 == "Release" ] && ([ $CI_BUILD_REF_NAME == "master" ] || [ -n "$CI_BUILD_TAG" ]); then
		if [ $? -ne 0 ]; then exit 1; fi

		# Make sure directory exists
		mkdir -p $DEPLOYDIR/Windows
		if [ $? -ne 0 ]; then exit 1; fi

		# D3D copies only EXE, OpenGL copies everything else
		if [ $3 == "--gfxapi=d3d11" ]; then
			cp -f builds/vaultClient_d3d11.exe $DEPLOYDIR/Windows/vaultClient_d3d11.exe
			if [ $? -ne 0 ]; then exit 1; fi
		else
			cp -f bin/sdl/SDL2.dll $DEPLOYDIR/Windows/SDL2.dll
			if [ $? -ne 0 ]; then exit 1; fi

			cp -f builds/vaultClient.exe $DEPLOYDIR/Windows/vaultClient.exe
			if [ $? -ne 0 ]; then exit 1; fi

			cp -rf builds/assets/ $DEPLOYDIR/Windows/assets
			if [ $? -ne 0 ]; then exit 1; fi

			cp -f $VAULTSDK_HOME/lib/win_x64/vaultSDK.dll $DEPLOYDIR/Windows/vaultSDK.dll
			if [ $? -ne 0 ]; then exit 1; fi
		fi
	fi
else
	if ([[ $OSTYPE == "darwin"* ]] && [ $3 == "ios" ]); then # iOS
		export OSNAME="iOS_$2"
		export BINARYSUFFIX="osx"
		bin/premake/premake5-osx xcode4 --os=ios
		if [ $? -ne 0 ]; then exit 1; fi
		if [ $2 == "x86_64" ]; then # Simulator
			xcodebuild -project vaultClient.xcodeproj -configuration $1 -arch $2 -sdk iphonesimulator
		else
			xcodebuild -project vaultClient.xcodeproj -configuration $1 -arch $2
		fi
	elif [[ $OSTYPE == "darwin"* ]]; then # OSX
		export OSNAME="OSX"
		export BINARYSUFFIX="osx"
		bin/premake/premake5-osx xcode4
		if [ $? -ne 0 ]; then exit 1; fi
		xcodebuild -project vaultClient.xcodeproj -configuration $1
	else
		export OSNAME="Linux"
		if [ $1 == "Release" ]; then
			export BINARYSUFFIX="linux_GCC"
		else
			export BINARYSUFFIX="linux_Clang"
		fi

		bin/premake/premake5 gmake
		if [ $? -ne 0 ]; then exit 1; fi
		make config=$(echo ${1}_${2} | tr [:upper:] [:lower:]) -j4
	fi
	if [ $? -ne 0 ]; then exit 1; fi

	if ([ $CI_BUILD_REF_NAME == "master" ] || [ -n "$CI_BUILD_TAG" ]); then
		# We build for both GCC and Clang, so need to handle them seperately
		if [ $1 == "Release" ] ; then
			# Make sure directory exists
			mkdir -p $DEPLOYDIR/$OSNAME
			if [ $? -ne 0 ]; then exit 1; fi

			if ([[ $OSTYPE == "darwin"* ]] && [ $3 == "ios" ]); then # iOS
				# Make folder to store the framework to build a DMG from
				mkdir builds/packaging
				if [ $? -ne 0 ]; then exit 1; fi

				cp -af builds/vaultClient.app builds/packaging/vaultClient.app
				if [ $? -ne 0 ]; then exit 1; fi

				hdiutil create builds/vaultClient.dmg -volname "vaultClient" -srcfolder builds/packaging

				cp -f builds/vaultClient.dmg $DEPLOYDIR/$OSNAME
			elif [[ $OSTYPE == "darwin"* ]]; then # OSX
				# Make folder to store the framework to build a DMG from
				mkdir -p builds/packaging/.background
				if [ $? -ne 0 ]; then exit 1; fi

				cp -af builds/vaultClient.app builds/packaging/vaultClient.app
				if [ $? -ne 0 ]; then exit 1; fi

				cp icons/dmgBackground.png builds/packaging/.background/background.png
				if [ $? -ne 0 ]; then exit 1; fi

				cp icons/macOSAppIcons.icns builds/packaging/.VolumeIcon.icns
				if [ $? -ne 0 ]; then exit 1; fi

				# See https://stackoverflow.com/a/1513578 for reference
				hdiutil create builds/vaultClient.temp.dmg -volname "vaultClient" -srcfolder builds/packaging -format UDRW -fs HFS+ -fsargs "-c c=64,a=16,e=16"
				device=$(hdiutil attach -readwrite -noverify -noautoopen "builds/vaultClient.temp.dmg" | egrep '^/dev/' | sed 1q | awk '{print $1}')
				echo '
					tell application "Finder"
						tell disk "'vaultClient'"
							open
							set current view of container window to icon view
							set toolbar visible of container window to false
							set statusbar visible of container window to false
							set the bounds of container window to {400, 100, (400 + 512), (100 + 320 + 23)}
							set theViewOptions to the icon view options of container window
							set arrangement of theViewOptions to not arranged
							set icon size of theViewOptions to 72
							set background picture of theViewOptions to file ".background:'background.png'"
							make new alias file at container window to POSIX file "/Applications" with properties {name:"Applications"}
							set position of item "'vaultClient'" of container window to {150, 180}
							set position of item "Applications" of container window to {360, 180}
							update without registering applications
							delay 5
							close
						end tell
					end tell
				' | osascript
				chmod -Rf go-w /Volumes/vaultClient
				SetFile -c icnC "/Volumes/vaultClient/.VolumeIcon.icns"
				SetFile -a C /Volumes/vaultClient
				sync
				sync
				hdiutil detach ${device}
				hdiutil convert "builds/vaultClient.temp.dmg" -format UDZO -imagekey zlib-level=9 -o "builds/vaultClient.dmg"
				rm -f builds/vaultClient.temp.dmg
				rm -rf builds/packaging

				cp -f builds/vaultClient.dmg $DEPLOYDIR/$OSNAME
			else
				sharedLibExtension="so"
				cp -rf builds/assets/ $DEPLOYDIR/$OSNAME/assets
				if [ $? -ne 0 ]; then exit 1; fi

				cp -f builds/vaultClient $DEPLOYDIR/$OSNAME/vaultClient
				if [ $? -ne 0 ]; then exit 1; fi
				cp -f $VAULTSDK_HOME/lib/linux_GCC_x64/libvaultSDK.so $DEPLOYDIR/$OSNAME/libvaultSDK.so
			fi

			if [ $? -ne 0 ]; then exit 1; fi
		fi
	fi
fi
