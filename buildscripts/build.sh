git clean -ffxd
git submodule foreach --recursive 'git clean -ffdx'

git submodule sync --recursive
git submodule update --init --recursive
if [ $? -ne 0 ]; then exit 1; fi

if [ $OSTYPE == "msys" ]; then # Windows, MingW
	export VAULTSDK_HOME="//bne-fs-fs-002.euclideon.local/Resources/Builds/euVault/sdk/Pipeline_21135"
	
	bin/premake/premake5.exe vs2015
	if [ $? -ne 0 ]; then exit 1; fi

	"C:/Program Files (x86)/MSBuild/14.0/Bin/amd64/MSBuild.exe" vaultClient.sln //p:Configuration=$1 //p:Platform=$2 //v:m //m
	if [ $? -ne 0 ]; then exit 1; fi

	if [ $1 == "Release" ] && (true || [ -n "$CI_BUILD_TAG" ]); then
		if [ $? -ne 0 ]; then exit 1; fi

		#NSIS
		# "cutil/bin/nsis/makensis.exe" buildscripts\windowsinstallerscript.nsi

		# Make sure directory exists
		mkdir -p //bne-fs-fs-002.euclideon.local/Resources/Builds/euVault/client/Pipeline_$CI_PIPELINE_ID/Windows
		if [ $? -ne 0 ]; then exit 1; fi

		cp -f builds/client/bin/vaultClient.exe //bne-fs-fs-002.euclideon.local/Resources/Builds/euVault/client/Pipeline_$CI_PIPELINE_ID/Windows/vaultClient.exe
		if [ $? -ne 0 ]; then exit 1; fi
		cp -f $VAULTSDK_HOME/Lib/Windows/vaultSDK.dll //bne-fs-fs-002.euclideon.local/Resources/Builds/euVault/client/Pipeline_$CI_PIPELINE_ID/Windows/vaultSDK.dll
		if [ $? -ne 0 ]; then exit 1; fi

	fi
else
	export VAULTSDK_HOME="/mnt/Resources/Builds/euVault/sdk/Pipeline_21135"

	if [ $OSTYPE == "darwin16" ]; then # OSX, Sierra
		export OSNAME="OSX"
		export BINARYSUFFIX="osx"
		bin/premake/premake5-osx gmake
	else
		export OSNAME="Linux"
		if [ $1 == "Release" ]; then
			export BINARYSUFFIX="linux_GCC"
		else
			export BINARYSUFFIX="linux_Clang"
		fi

		bin/premake/premake5 gmake
	fi
	if [ $? -ne 0 ]; then exit 1; fi

	make config=$(echo ${1}_${2} | tr [:upper:] [:lower:]) -j4
	if [ $? -ne 0 ]; then exit 1; fi

	if (true || [ -n "$CI_BUILD_TAG" ]); then
		# We build for both GCC and Clang, so need to handle them seperately
		if [ $1 == "Release" ] ; then
			# Make sure directory exists
			mkdir -p /mnt/Resources/Builds/euVault/client/Pipeline_$CI_PIPELINE_ID/$OSNAME
			if [ $? -ne 0 ]; then exit 1; fi

			cp -f builds/client/bin/vaultClient /mnt/Resources/Builds/euVault/client/Pipeline_$CI_PIPELINE_ID/$OSNAME/vaultClient
			if [ $? -ne 0 ]; then exit 1; fi

			local sharedLibExtension=""
			if [ $OSTYPE == "darwin16" ]; then # OSX, Sierra
				sharedLibExtension="dylib"
			else
				sharedLibExtension="so"
			fi

			cp -f $VAULTSDK_HOME/Lib/$OSNAME/libvaultSDK.$sharedLibExtension /mnt/Resources/Builds/euVault/client/Pipeline_$CI_PIPELINE_ID/$OSNAME/libvaultSDK.$sharedLibExtension
			if [ $? -ne 0 ]; then exit 1; fi
		fi
	fi
fi
