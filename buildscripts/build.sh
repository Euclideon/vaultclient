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
export VAULTSDK_HOME="$RESOURCES/Builds/vault/sdk/Pipeline_25013"

if [ $OSTYPE == "msys" ]; then # Windows, MinGW
	bin/premake/premake5.exe vs2015
	if [ $? -ne 0 ]; then exit 1; fi

	"C:/Program Files (x86)/MSBuild/14.0/Bin/amd64/MSBuild.exe" vaultClient.sln //p:Configuration=$1 //p:Platform=$2 //v:m //m
	if [ $? -ne 0 ]; then exit 1; fi

	if [ $1 == "Release" ] && ([ $CI_BUILD_REF_NAME == "master" ] || [ -n "$CI_BUILD_TAG" ]); then
		if [ $? -ne 0 ]; then exit 1; fi

		# Make sure directory exists
		mkdir -p $DEPLOYDIR/Windows
		if [ $? -ne 0 ]; then exit 1; fi

		cp -f bin/sdl/SDL2.dll $DEPLOYDIR/Windows/SDL2.dll
		if [ $? -ne 0 ]; then exit 1; fi

		cp -f builds/vaultClient.exe $DEPLOYDIR/Windows/vaultClient.exe
		if [ $? -ne 0 ]; then exit 1; fi

		cp -rf builds/assets/ $DEPLOYDIR/Windows/assets
		if [ $? -ne 0 ]; then exit 1; fi

		cp -f $VAULTSDK_HOME/lib/win_x64/vaultSDK.dll $DEPLOYDIR/Windows/vaultSDK.dll
		if [ $? -ne 0 ]; then exit 1; fi

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

			if [[ $OSTYPE == "darwin"* ]]; then # OSX
				# Make folder to store the framework to build a DMG from
				mkdir builds/packaging
				if [ $? -ne 0 ]; then exit 1; fi

				cp -af builds/vaultClient.app builds/packaging/vaultClient.app
				if [ $? -ne 0 ]; then exit 1; fi

				hdiutil create builds/vaultClient.dmg -volname "vaultClient" -srcfolder builds/packaging

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
