require "vstudio"
require "emscripten"

table.insert(premake.option.get("os").allowed, { "ios", "Apple iOS" })
table.insert(premake.option.get("os").allowed, { "emscripten", "Emscripten" })

-- TODO: Move this into the emscripten module
function premake.modules.emscripten.emcc.getrunpathdirs(cfg, dirs)
	return premake.tools.clang.getrunpathdirs(cfg, dirs)
end

filter { "system:ios", "action:xcode4" }
	xcodebuildsettings {
		['PRODUCT_BUNDLE_IDENTIFIER'] = 'Euclideon udStream',
		["CODE_SIGN_IDENTITY[sdk=iphoneos*]"] = "iPhone Developer",
		['IPHONEOS_DEPLOYMENT_TARGET'] = '10.3',
		['SDKROOT'] = 'iphoneos',
		['ARCHS'] = 'arm64',
		['TARGETED_DEVICE_FAMILY'] = "1,2",
		['DEVELOPMENT_TEAM'] = "452P989JPT",
		['ENABLE_BITCODE'] = "NO",
	}

function getosinfo()
	local osname = os.target()
	local distroExtension = ""
	if os.target() == premake.LINUX then
		osname = os.outputof('lsb_release -ir | head -n2 | cut -d ":" -f 2 | tr -d "\n\t" | tr [:upper:] [:lower:]')
		if osname:startswith("centos") then
			osname = os.outputof('lsb_release -ir | head -n2 | cut -d ":" -f 2 | tr -d "\n\t" | tr [:upper:] [:lower:] | cut -d "." -f 1')
			distroExtension = "_rpm"
		else
			distroExtension = "_deb"
		end
	end

	return osname, distroExtension
end

function ProcessudSDK()
	-- Calculate the paths
	local osname, distroExtension = getosinfo()

	if os.target() == premake.MACOSX then
		links { "udSDK.framework" }
	elseif os.target() == "emscripten" and not _OPTIONS["force-udSDK"] then
		linkoptions { "src/libudSDK.bc", "src/libudPointCloud.bc", "src/libvaultcore.bc" }
	else
		links { "udSDK" }
	end

	if _OPTIONS["force-udSDK"] then
		if udSDKLocalEmbed ~= nil then
			udSDKLocalEmbed()
		else
			error "Attempting to link with an unsupported version of udSDK!"
		end
	else
		if _OPTIONS["udSDK"] == nil then
			error "udSDK not installed correctly. (No UDSDK_HOME environment variable or --udSDK=[PATH] argument!)"
		end

		includedirs(_OPTIONS["udSDK"] .. "/include")

		if os.target() == premake.WINDOWS then
			if _OPTIONS["gfxapi"] == "metal" then
				_OPTIONS["gfxapi"] = "opengl"
			end
			os.copyfile(_OPTIONS["udSDK"] .. "/lib/win_x64/udSDK.dll", "builds/udSDK.dll")
			libdirs { _OPTIONS["udSDK"] .. "/lib/win_x64/" }
		elseif os.target() == premake.MACOSX then
			os.execute("mkdir -p builds")

			-- copy dmg, mount, extract framework, unmount then remove.
			os.copyfile(_OPTIONS["udSDK"] .. "/lib/osx_x64/udSDK.dmg", "builds/udSDK.dmg")
			local device = os.outputof("/usr/bin/hdiutil attach -noverify -noautoopen builds/udSDK.dmg | egrep '^/dev/' | sed 1q | awk '{print $1}'")
			os.execute("cp -a -f /Volumes/udSDK/udSDK.framework builds/")
			os.execute("/usr/bin/hdiutil detach /Volumes/udSDK")
			os.execute("/usr/bin/hdiutil detach " .. device)
			os.execute("rm -r builds/udSDK.dmg")
			prelinkcommands {
				"rm -rf %{prj.targetdir}/%{prj.targetname}.app/Contents/Frameworks",
				"mkdir -p %{prj.targetdir}/%{prj.targetname}.app/Contents/Frameworks",
				"cp -af builds/udSDK.framework %{prj.targetdir}/%{prj.targetname}.app/Contents/Frameworks/",
				"cp -af /Library/Frameworks/SDL2.framework %{prj.targetdir}/%{prj.targetname}.app/Contents/Frameworks/",
			}
			linkoptions { "-rpath @executable_path/../Frameworks/" }
			frameworkdirs { "builds" }
		elseif os.target() == premake.IOS then
			os.execute("mkdir -p builds")
			os.execute("lipo -create " .. _OPTIONS["udSDK"] .. "/lib/ios_arm64/libudSDK.dylib " .. _OPTIONS["udSDK"] .. "/lib/ios_x64/libudSDK.dylib -output builds/libudSDK.dylib")
			os.execute("codesign -s T6Q3JCVW77 builds/libudSDK.dylib") -- Is this required? Should this move to VaultSDK?
			libdirs { "builds" }
			linkoptions { "-rpath @executable_path/" }
		elseif os.target() == "emscripten" then
			os.copyfile(_OPTIONS["udSDK"] .. "/lib/emscripten_wasm32/libudSDK.bc", "src/libudSDK.bc")
			os.copyfile(_OPTIONS["udSDK"] .. "/lib/emscripten_wasm32/libvaultcore.bc", "src/libvaultcore.bc")
			os.copyfile(_OPTIONS["udSDK"] .. "/lib/emscripten_wasm32/libudPointCloud.bc", "src/libudPointCloud.bc")
			os.copyfile(_OPTIONS["udSDK"] .. "/lib/emscripten_wasm32/vCore.h", "src/vCore.h")
			os.copyfile(_OPTIONS["udSDK"] .. "/lib/emscripten_wasm32/vHTTP.h", "src/vHTTP.h")
			os.copyfile(_OPTIONS["udSDK"] .. "/lib/emscripten_wasm32/vHTTPRequest.h", "src/vHTTPRequest.h")
			os.copyfile(_OPTIONS["udSDK"] .. "/lib/emscripten_wasm32/vHTTPRequest.js", "src/vHTTPRequest.js")
			libdirs { "src" }
			linkoptions  { "--js-library src/vHTTPRequest.js" }
		elseif os.target() == premake.ANDROID then
			filter { "architecture:x64" }
				libdirs { _OPTIONS["udSDK"] .. "/lib/android_x64" }
			filter { "architecture:arm64" }
				libdirs { _OPTIONS["udSDK"] .. "/lib/android_arm64" }
			filter {}
		else
			if _OPTIONS["gfxapi"] == "metal" then
				_OPTIONS["gfxapi"] = "opengl"
			end
			os.execute("mkdir -p builds")
			os.copyfile(_OPTIONS["udSDK"] .. "/lib/" .. osname .. "_GCC_x64/libudSDK.so", "builds/libudSDK.so")
			libdirs { "builds" }
		end
	end
end

newoption {
	trigger     = "force-udSDK",
	description = "Force the use of the udSDK repository"
}

if os.target() == premake.MACOSX or os.target() == premake.IOS then
	gfxDefault = "metal"
elseif os.target() == premake.WINDOWS then
	gfxDefault = "d3d11"
else
	gfxDefault = "opengl"
end

newoption {
	trigger     = "gfxapi",
	value       = "API",
	description = "Choose a particular 3D API for rendering",
	default     = gfxDefault,
	allowed = {
		{ "opengl", "OpenGL" },
		{ "d3d11", "Direct3D 11 (Windows only)" },
		{ "metal", "Metal (macOS & iOS only)" }
	}
}

newoption {
	trigger     = "udSDK",
	value       = "Path",
	description = "Path to udSDK",
	default     = os.getenv("UDSDK_HOME")
}

if os.target() == premake.WINDOWS then
	fbxDefault = "C:/Program Files/Autodesk/FBX/FBX SDK/2019.5"
elseif os.target() == premake.MACOSX then
	fbxDefault = "/Applications/Autodesk/FBX SDK/2019.5"
--elseif os.target() == premake.LINUX --This isn't properly tested or supported yet
--	fbxDefault = "/usr/FBX20195_FBXFILESDK_LINUX"
else
	fbxDefault = nil
end

newoption {
	trigger     = "fbxsdk",
	value       = "Path",
	description = "Path to FBX SDK",
	default     = fbxDefault
}

if _ACTION == "xcode4" and os.target() == premake.MACOSX then
	_OPTIONS["fbxsdk"] = _OPTIONS["fbxsdk"]:gsub(" ", "\\ ")
end

solution "udStream"
	-- This hack just makes the VS project and also the makefile output their configurations in the idiomatic order
	if (_ACTION == "gmake" or _ACTION == "gmake2") and os.target() == "linux" then
		configurations { "Release", "Debug", "ReleaseClang", "DebugClang" }
		linkgroups "On"
		filter { "configurations:*Clang" }
			toolset "clang"
		filter { }
	elseif os.target() == "macosx" or os.target() == "ios" then
		configurations { "Release", "Debug" }
		toolset "clang"
	else
		configurations { "Debug", "Release" }
	end

	if os.target() == "emscripten" then
		platforms { "Emscripten" }
		buildoptions { "-s USE_SDL=2", "-s USE_PTHREADS=1", "-s PTHREAD_POOL_SIZE=20", "-s PROXY_TO_PTHREAD=1", --[["-s OFFSCREEN_FRAMEBUFFER=1",]] "-s OFFSCREENCANVAS_SUPPORT=1", --[["-s ASSERTIONS=2",]] "-s EMULATE_FUNCTION_POINTER_CASTS=1", "-s ABORTING_MALLOC=0", "-s WASM=1", "-mnontrapping-fptoint", "-s FORCE_FILESYSTEM=1", --[["-g", "-s SAFE_HEAP=1",]] "-s TOTAL_MEMORY=2147418112" }
		linkoptions  { "-s USE_SDL=2", "-s USE_PTHREADS=1", "-s PTHREAD_POOL_SIZE=20", "-s PROXY_TO_PTHREAD=1", --[["-s OFFSCREEN_FRAMEBUFFER=1",]] "-s OFFSCREENCANVAS_SUPPORT=1", --[["-s ASSERTIONS=2",]] "-s EMULATE_FUNCTION_POINTER_CASTS=1", "-s ABORTING_MALLOC=0", "-s WASM=1", "-mnontrapping-fptoint", "-s FORCE_FILESYSTEM=1", "-lidbfs.js", --[["-g", "-s SAFE_HEAP=1",]] "-s TOTAL_MEMORY=2147418112" }
		linkoptions { "-O3" } -- TODO: This might not be required once `-s PROXY_TO_PTHREAD` is working.
		targetextension ".bc"
		linkgroups "On"
		filter { "kind:*App" }
			targetextension ".js"
		filter { "files:**.cpp" }
			buildoptions { "-std=c++14" }
		filter { "system:emscripten" }
			disablewarnings "string-plus-int"
		filter {}
	elseif os.target() == "android" or os.target() == "ios" then
		platforms { "x64", "arm64" }
	else
		platforms { "x64" }
	end

	editorintegration "on"
	startproject "udStream"
	cppdialect "C++14"
	pic "On"
	editandcontinue "Off"

	filter { "system:emscripten" }
		pic "Off"
	filter { "system:windows", "action:vs*" }
		buildoptions { "/permissive-" }
	filter {}

	if os.target() == premake.WINDOWS then
		os.copyfile("3rdParty/SDL2-2.0.8/lib/x64/SDL2.dll", "builds/SDL2.dll")
	end

	if os.getenv("BUILD_BUILDNUMBER") then
		defines { "GIT_BUILD=" .. os.getenv("BUILD_BUILDNUMBER") }
		xcodebuildsettings { ["INFOPLIST_PREPROCESSOR_DEFINITIONS"] = "GIT_BUILD=" .. os.getenv("BUILD_BUILDNUMBER") }

		if os.getenv("BUILD_SOURCEBRANCH") and os.getenv("BUILD_SOURCEBRANCH"):startswith("refs/tags/") then
			defines { "GIT_TAG=" .. os.getenv("BUILD_BUILDNUMBER") }
		end
	end

	-- Uncomment this to help with finding memory leaks
	--defines {"__MEMORY_DEBUG__"}

	if _OPTIONS["force-udSDK"] then
		dofile "../udsdk/udSDKImport.lua"
	end

	if not (os.target() == "emscripten" and _OPTIONS["force-udSDK"]) then
		dofile "3rdParty/udcore/project.lua"
	end

	filter {}
	removeflags { "FatalWarnings" }

	dofile "vcGL/project.lua"
	dofile "project.lua"

	if os.target() == "android" then
		dofile "buildscripts/android/packaging.lua"
	end

	if os.target() ~= premake.IOS and os.target() ~= premake.ANDROID and os.target() ~= "emscripten" then
		dofile "vcConvertCMD/project.lua"
		dofile "3rdParty/udcore/3rdParty/googletest/project.lua"
		dofile "vcTesting/project.lua"
	end

