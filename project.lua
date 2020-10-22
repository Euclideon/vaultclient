project "udStream"
	--Settings
	kind "WindowedApp"

	language "C++"
	staticruntime "On"
	flags { "FatalWarnings", "MultiProcessorCompile" }

	--Files to include
	files { "src/**.cpp", "src/**.h", "src/**.c", "src/**.mm" }
	files { "src/**.hlsl", "src/**.vert", "src/**.frag", "src/**.metal" }
	files { "3rdParty/imgui/*.cpp", "3rdParty/imgui/*.h" }
	files { "3rdParty/stb/**.h" }
	files { "3rdParty/easyexif/**.h", "3rdParty/easyexif/**.cpp" }
	files { "project.lua" }
	files { "docs/**.md" }
	files { "builds/releasenotes.md" }
	files { "builds/defaultsettings.json" }
	files { "builds/assets/lang/*.json" }
	files { "builds/assets/branding/*.json" }
	files { "builds/assets/guide/**" }
	files { "3rdParty/atmosphere/atmosphere/**", "3rdParty/atmosphere/external/dimensional_types/math/**" }
	files { "3rdParty/poly2tri/**.h", "3rdParty/poly2tri/**.cpp" }
	files { "3rdParty/LercLib/**.h", "3rdParty/LercLib/**.hpp", "3rdParty/LercLib/**.cpp" }

	--This project includes
	includedirs { "src", "src/scene", "src/rendering" }
	includedirs { "3rdParty/udcore/Include" }
	includedirs { "3rdParty/imgui" }
	includedirs { "3rdParty/libtiff/libtiff" }
	includedirs { "3rdParty/imgui_markdown" }
	includedirs { "3rdParty/udcore/3rdParty/stb" }
	includedirs { "3rdParty/easyexif" }
	includedirs { "3rdParty/poly2tri" }
	includedirs { "3rdParty/atmosphere", "3rdParty/atmosphere/external/dimensional_types" }
	includedirs { "3rdParty/LercLib" }
	includedirs { "vcGL/src" }	

	links { "udCore" .. (projectSuffix or ""), "vcGL", "libtiff" }

	defines { "IMGUI_DISABLE_OBSOLETE_FUNCTIONS", "ImDrawIdx=int" }

	ProcessudSDK()

	local excludedSourceFileNames = {}
	if _OPTIONS["gfxapi"] ~= "opengl" then
		table.insert(excludedSourceFileNames, "src/gl/opengl/*");
	end
	if _OPTIONS["gfxapi"] ~= "d3d11" then
		table.insert(excludedSourceFileNames, "src/gl/directx11/*");
	end
	if _OPTIONS["gfxapi"] ~= "metal" then
		table.insert(excludedSourceFileNames, "src/gl/metal/*");
		table.insert(excludedSourceFileNames, "src/imgui_ex/*.mm");
	end
	if os.target() == premake.IOS then
		table.insert(excludedSourceFileNames, "src/gl/opengl/shaders/desktop/*");
		table.insert(excludedSourceFileNames, "src/gl/metal/shaders/desktop/*");
	end
	if os.target() ~= premake.IOS then
		table.insert(excludedSourceFileNames, "src/gl/opengl/shaders/mobile/*");
		table.insert(excludedSourceFileNames, "src/gl/metal/shaders/mobile/*");
		table.insert(excludedSourceFileNames, "src/vcWebFile.mm");
	end

	if (_OPTIONS["fbxsdk"] ~= nil and os.isdir(_OPTIONS["fbxsdk"])) then
		defines { "FBXSDK_ON", "FBXSDK_SHARED" }
		includedirs { '%{_OPTIONS["fbxsdk"]}/include/' }

		filter { "system:windows" }
			links { "libfbxsdk.lib" }
			libdirs { '%{_OPTIONS["fbxsdk"]}/lib/vs2017/x64/%{cfg.buildcfg}/' }
			prelinkcommands {
				'copy /B "' .. path.translate(_OPTIONS["fbxsdk"] .. '\\lib\\vs2017\\x64\\%{cfg.buildcfg}\\libfbxsdk.dll') .. '" %{prj.targetdir}'
			}

		filter { "system:macosx" }
			links { "fbxsdk" }
			libdirs { '"' .. _OPTIONS["fbxsdk"] .. '"/lib/clang/%{cfg.buildcfg}"' }
			xcodebuildsettings {
				["GCC_ENABLE_CPP_EXCEPTIONS"] = "YES",
				["SYSTEM_HEADER_SEARCH_PATHS"] = '"' .. _OPTIONS["fbxsdk"] .. '/include/"'
			}
			postbuildcommands {
				-- Note that this copies the dylib directly into the .app which is not ideal
				'cp -af "' .. _OPTIONS["fbxsdk"]:gsub("\\ ", " ") .. '/lib/clang/release/libfbxsdk.dylib" %{prj.targetdir}/%{prj.targetname}.app/Contents/MacOS'
			}

		filter { "system:linux" }
			links { "fbxsdk.so" }
			prelinkcommands {
				"cp -af \"" .. _OPTIONS["fbxsdk"] .. "/lib/gcc/x64/%{cfg.buildcfg}/libfbxsdk.so\" %{prj.targetdir}"
			}
	end

	-- filters
	filter { "configurations:Debug" }
		kind "ConsoleApp"
		optimize "Debug"
		removeflags { "FatalWarnings" }

	filter { "configurations:Debug", "system:Windows" }
		ignoredefaultlibraries { "libcmt" }

	filter { "configurations:Release" }
		optimize "Full"
		omitframepointer "On"
		flags { "NoBufferSecurityCheck" }

	filter { "system:windows" }
		sysincludedirs { "3rdParty/SDL2-2.0.8/include" }
		files { "src/**.rc" }
		linkoptions( "/LARGEADDRESSAWARE" )
		libdirs { "3rdParty/SDL2-2.0.8/lib/x64" }
		links { "SDL2.lib", "SDL2main.lib", "winmm.lib", "ws2_32", "winhttp", "imm32.lib" }
		dpiawareness "HighPerMonitor"

	filter { "system:linux" }
		linkoptions { "-Wl,-rpath '-Wl,$$ORIGIN'" } -- Check beside the executable for the SDK
		linkoptions { "-no-pie" } -- Enable double click on binary in Ubuntu
		links { "SDL2", "GL", "z" }
		includedirs { "3rdParty" }
		files { "3rdParty/GL/glext.h" }

	-- Clang on Ubuntu 16.04 doesn't support -no-pie
	if os.host() == premake.LINUX then
		status = os.execute('echo "int main() { return 0; }" | clang -o a.out -no-pie -xc - > /dev/null 2>2&1')
		if not status then
			filter { "system:linux", "toolset:clang" }
				removelinkoptions { "-no-pie" }
			filter {}
		end
		os.execute("rm a.out")
	end

	filter { "system:android" }
		links { "m", "android", "SDL2", "GLESv3" }
		sysincludedirs { "3rdParty/SDL2-2.0.8/include" }
		includedirs { "3rdParty" }
		libdirs { "3rdParty/SDL2-2.0.8/lib/android/%{cfg.platform}" }
		kind "SharedLib"

	filter { "system:macosx" }
		files { "macOS-Info.plist", "icons/macOSAppIcons.icns" }
		frameworkdirs { "/Library/Frameworks/" }
		links { "SDL2.framework", "OpenGL.framework" }
		xcodebuildsettings {
			["INFOPLIST_PREFIX_HEADER"] = "src/vcVersion.h",
			["INFOPLIST_PREPROCESS"] = "YES",
			["MACOSX_DEPLOYMENT_TARGET"] = "10.13",
		}

	filter { "system:ios" }
		files { "iOS-Info.plist", "builds/libudSDK.dylib", "icons/Images.xcassets", "src/vcWebFile.mm" }
		sysincludedirs { "3rdParty/SDL2-2.0.8/include" }
		xcodebuildresources { "libudSDK", "Images.xcassets" }
		xcodebuildsettings { ["ASSETCATALOG_COMPILER_APPICON_NAME"] = "AppIcon" }
		removefiles { "3rdParty/glew/glew.c" }
		libdirs { "3rdParty/SDL2-2.0.8/lib/ios" }
		links { "SDL2", "AudioToolbox.framework", "QuartzCore.framework", "OpenGLES.framework", "CoreGraphics.framework", "UIKit.framework", "Foundation.framework", "CoreAudio.framework", "AVFoundation.framework", "GameController.framework", "CoreMotion.framework" }

	filter { "system:macosx or ios" }
		files { "builds/assets/**", "builds/releasenotes.md", "builds/defaultsettings.json" }
		xcodebuildresources { "^builds/assets/.*%..*", "releasenotes", "defaultsettings" }
		xcodebuildsettings { ["EXCLUDED_SOURCE_FILE_NAMES"] = excludedSourceFileNames }

	filter { "system:emscripten" }
		kind "ConsoleApp" -- WindowedApp does not generate the final output
		links { "GLEW" }
		linkoptions  { "-s USE_WEBGL2=1", "-s FULL_ES3=1", --[["-s DEMANGLE_SUPPORT=1",]] "-s EXTRA_EXPORTED_RUNTIME_METHODS='[\"ccall\", \"cwrap\", \"getValue\", \"setValue\", \"UTF8ToString\", \"stringToUTF8\"]'" }

	filter { "system:emscripten", "options:force-udsdk" }
		removeincludedirs { "3rdParty/udcore/Include" }
		removelinks { "udCore" .. (projectSuffix or "") }

	filter { "system:emscripten", "options:not force-udsdk" }
		removefiles { "src/vCore/vUUID.cpp", "src/vCore/vWorkerThread.cpp" }

	filter { "system:not windows" }
		links { "dl" }

	filter { "options:gfxapi=opengl" }
		defines { "GRAPHICS_API_OPENGL=1" }

	filter { "options:gfxapi=opengl", "system:emscripten" }
		prebuildcommands {
			"rm -rf builds/assets/shaders",
			"mkdir builds/assets/shaders",
			"cp src/gl/opengl/shaders/mobile/* builds/assets/shaders",
		}

	filter { "options:gfxapi=opengl", "system:linux" }
		prebuildcommands {
			"rm -rf builds/assets/shaders",
			"mkdir builds/assets/shaders",
			"cp src/gl/opengl/shaders/desktop/* builds/assets/shaders",
		}

	filter { "options:gfxapi=opengl", "system:macosx or ios" }
		xcodebuildresources { "%.vert$", "%.frag$" }

	filter { "options:gfxapi=opengl", "system:android" }
		prebuildcommands {
			"rmdir builds\\assets\\shaders /S /Q",
			"xcopy src\\gl\\opengl\\shaders\\mobile builds\\assets\\shaders /I",
		}

	filter { "options:gfxapi=opengl", "system:windows" }
		links { "opengl32.lib" }
		prebuildcommands {
			"rmdir builds\\assets\\shaders /S /Q",
			"xcopy src\\gl\\opengl\\shaders\\desktop builds\\assets\\shaders /I",
		}

	filter { "options:gfxapi=d3d11" }
		libdirs { "$(DXSDK_DIR)/Lib/x64;" }
		links { "d3d11.lib", "d3dcompiler.lib", "dxgi.lib", "dxguid.lib" }
		defines { "GRAPHICS_API_D3D11=1" }
		prebuildcommands {
			"rmdir builds\\assets\\shaders /S /Q",
			"xcopy src\\gl\\directx11\\shaders builds\\assets\\shaders /I",
		}

	filter { "options:gfxapi=metal" }
		defines { "GRAPHICS_API_METAL=1" }
		xcodebuildresources { "%.metal$" }
		xcodebuildsettings {
			["CLANG_ENABLE_OBJC_ARC"] = "YES",
			["GCC_ENABLE_OBJC_EXCEPTIONS"] = "YES",
		}
		links { "MetalKit.framework", "Metal.framework" }

	filter { "options:gfxapi=metal", "system:macosx" }
		links { "AppKit.framework", "QuartzCore.framework" }

	filter { "options:not gfxapi=opengl", "files:src/gl/opengl/*", "system:not macosx" }
		flags { "ExcludeFromBuild" }
	filter { "options:not gfxapi=d3d11", "files:src/gl/directx11/*", "system:not macosx" }
		flags { "ExcludeFromBuild" }
	filter { "options:not gfxapi=metal", "files:src/gl/metal/*", "system:not macosx" }
		flags { "ExcludeFromBuild" }
	filter { "files:**.hlsl" }
		flags { "ExcludeFromBuild" }

	filter { "system:not ios", "system:not macosx", "files:src/**.mm" }
		flags { "ExcludeFromBuild" }



	-- include common stuff
	dofile "3rdParty/udcore/bin/premake-bin/common-proj.lua"

	floatingpoint "default"

	filter { "system:ios or system:macosx" }
		removeflags { "FatalWarnings" }

	filter { "files:3rdParty/**" }
		warnings "Off"

	filter { "not options:gfxapi=" .. gfxDefault }
		objdir ("Output/intermediate/%{prj.name}/%{cfg.buildcfg}_%{cfg.platform}" .. _OPTIONS["gfxapi"])
		targetname ("%{prj.name}_" .. _OPTIONS["gfxapi"])

	filter {}

	symbols "On"
	targetdir "builds"
	debugdir "builds"
