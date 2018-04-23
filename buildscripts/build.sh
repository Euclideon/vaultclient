git clean -ffxd
git submodule foreach --recursive 'git clean -ffdx'

git submodule sync --recursive
git submodule update --init --recursive
if [ $? -ne 0 ]; then exit 1; fi

if [ $OSTYPE == "msys" ]; then # Windows, MingW
	export VAULTSDK_HOME="//bne-fs-fs-003.euclideon.local/Resources/Builds/vault/sdk/Pipeline_23795"
	
	bin/premake/premake5.exe vs2015
	if [ $? -ne 0 ]; then exit 1; fi

	"C:/Program Files (x86)/MSBuild/14.0/Bin/amd64/MSBuild.exe" vaultClient.sln //p:Configuration=$1 //p:Platform=$2 //v:m //m
	if [ $? -ne 0 ]; then exit 1; fi

	if [ $1 == "Release" ] && ([ $CI_BUILD_REF_NAME == "master" ] || [ -n "$CI_BUILD_TAG" ]); then
		if [ $? -ne 0 ]; then exit 1; fi

		#NSIS
		# "cutil/bin/nsis/makensis.exe" buildscripts\windowsinstallerscript.nsi

		# Make sure directory exists
		mkdir -p //bne-fs-fs-003.euclideon.local/Resources/Builds/vault/client/Pipeline_$CI_PIPELINE_ID/Windows
		if [ $? -ne 0 ]; then exit 1; fi

		cp -f builds/client/bin/vaultClient.exe //bne-fs-fs-003.euclideon.local/Resources/Builds/vault/client/Pipeline_$CI_PIPELINE_ID/Windows/vaultClient.exe
		if [ $? -ne 0 ]; then exit 1; fi
		cp -f $VAULTSDK_HOME/Lib/Windows/vaultSDK.dll //bne-fs-fs-003.euclideon.local/Resources/Builds/vault/client/Pipeline_$CI_PIPELINE_ID/Windows/vaultSDK.dll
		if [ $? -ne 0 ]; then exit 1; fi

	fi
else
	export VAULTSDK_HOME="/mnt/Resources/Builds/vault/sdk/Pipeline_23795"

	if [ $OSTYPE == "darwin16" ]; then # OSX, Sierra
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
			mkdir -p /mnt/Resources/Builds/vault/client/Pipeline_$CI_PIPELINE_ID/$OSNAME
			if [ $? -ne 0 ]; then exit 1; fi

			if [ $OSTYPE == "darwin16" ]; then # OSX, Sierra
				sharedLibExtension="dylib"
				cp -f -R builds/client/bin/vaultClient.app /mnt/Resources/Builds/vault/client/Pipeline_$CI_PIPELINE_ID/$OSNAME
			else
				sharedLibExtension="so"
				cp -f builds/client/bin/vaultClient /mnt/Resources/Builds/vault/client/Pipeline_$CI_PIPELINE_ID/$OSNAME/vaultClient
				if [ $? -ne 0 ]; then exit 1; fi
				cp -f $VAULTSDK_HOME/Lib/$OSNAME/libvaultSDK.so /mnt/Resources/Builds/vault/client/Pipeline_$CI_PIPELINE_ID/$OSNAME/libvaultSDK.so
			fi

			if [ $? -ne 0 ]; then exit 1; fi
		fi
	fi
fi
