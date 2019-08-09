set -xeu

git clean -ffxd
git submodule foreach --recursive 'git clean -ffdx'

git submodule sync --recursive
git submodule update --init --recursive

if [ $OSTYPE == "msys" ]; then # Windows, MinGW
	export DEV="//bne-fs-fs-003.euclideon.local/Dev"
else
	export DEV="/mnt/Dev"
fi

export DEPLOYDIR="$DEV/Builds/vault/client/Pipeline_$CI_PIPELINE_ID"

if [ $OSTYPE == "msys" ]; then # Windows, MinGW
	# Copy pre-package directory locally
	cp -rf $DEPLOYDIR/Windows ./package

	# Create package zip
	pushd package
	/c/Program\ Files/7-Zip/7z.exe a ..\\vaultclient.zip assets\\ userguide\\ SDL2.dll vaultClient.exe vaultClient_OpenGL.exe vaultConvertCMD.exe vaultSDK.dll releasenotes.md defaultsettings.json
	if [ -f "libfbxsdk.dll" ]; then
		/c/Program\ Files/7-Zip/7z.exe a ..\\vaultclient.zip libfbxsdk.dll
	fi
	popd

	# Deploy package zip
	cp -f vaultclient.zip  $DEPLOYDIR/vaultClient-Windows.zip

	# Remove pre-package directory
	rm -rf $DEPLOYDIR/Windows
fi
