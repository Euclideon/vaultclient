project "vaultClient"
	--Settings
	kind "WindowedApp"

	language "C++"
	flags { "StaticRuntime", "FatalWarnings", "MultiProcessorCompile" }

	--Files to include
	files { "src/**.cpp", "src/**.h", "src/**.c", "src/**.mm", "src/**.metal" }
	files { "3rdParty/Imgui/**.cpp", "3rdParty/Imgui/**.h" }
	files { "3rdParty/stb/**.h" }
	files { "3rdParty/easyexif/**.h", "3rdParty/easyexif/**.cpp" }
	files { "project.lua" }
	files { "docs/**.md" }
	files { "builds/releasenotes.md" }
	files { "builds/defaultsettings.json" }
	files { "builds/assets/lang/*.json" }

	--This project includes
	includedirs { "src", "src/scene", "src/rendering" }
	includedirs { "3rdParty/Imgui" }
	includedirs { "3rdParty/stb" }
	includedirs { "3rdParty/easyexif" }

	defines { "IMGUI_DISABLE_OBSOLETE_FUNCTIONS" }

	symbols "On"
	injectvaultsdkbin()

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
	if os.target() ~= premake.IOS then
		table.insert(excludedSourceFileNames, "src/vcWebFile.mm");
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
		flags { "NoFramePointer", "NoBufferSecurityCheck" }

	filter { "system:windows" }
		defines { "GLEW_STATIC" }
		sysincludedirs { "3rdParty/glew/include", "3rdParty/SDL2-2.0.8/include" }
		files { "3rdParty/glew/glew.c", "src/**.rc" }
		linkoptions( "/LARGEADDRESSAWARE" )
		libdirs { "3rdParty/SDL2-2.0.8/lib/x64" }
		links { "SDL2.lib", "SDL2main.lib", "opengl32.lib", "winmm.lib", "ws2_32", "winhttp" }

	filter { "system:linux" }
		linkoptions { "-Wl,-rpath '-Wl,$$ORIGIN'" } -- Check beside the executable for the SDK
		links { "SDL2", "GL" }
		includedirs { "3rdParty" }
		files { "3rdParty/GL/glext.h" }

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
		files { "iOS-Info.plist", "builds/libvaultSDK.dylib", "icons/Images.xcassets", "src/vcWebFile.mm" }
		sysincludedirs { "3rdParty/SDL2-2.0.8/include" }
		xcodebuildresources { "libvaultSDK", "Images.xcassets" }
		xcodebuildsettings { ["ASSETCATALOG_COMPILER_APPICON_NAME"] = "AppIcon" }
		removefiles { "3rdParty/glew/glew.c" }
		libdirs { "3rdParty/SDL2-2.0.8/lib/ios" }
		links { "SDL2", "AudioToolbox.framework", "QuartzCore.framework", "OpenGLES.framework", "CoreGraphics.framework", "UIKit.framework", "Foundation.framework", "CoreAudio.framework", "AVFoundation.framework", "GameController.framework", "CoreMotion.framework" }

	filter { "system:macosx or ios" }
		files { "builds/assets/**", "builds/releasenotes.md", "builds/defaultsettings.json" }
		xcodebuildresources { ".otf", ".png", ".jpg", ".json", ".metallib", "releasenotes", "defaultsettings" }
		xcodebuildsettings { ["EXCLUDED_SOURCE_FILE_NAMES"] = excludedSourceFileNames }

	filter { "system:not windows" }
		links { "dl" }

	filter { "system:linux" }
		links { "z" }

	filter { "options:gfxapi=opengl" }
		defines { "GRAPHICS_API_OPENGL=1" }

	filter { "options:gfxapi=d3d11" }
		libdirs { "$(DXSDK_DIR)/Lib/x64;" }
		links { "d3d11.lib", "d3dcompiler.lib", "dxgi.lib", "dxguid.lib" }
		defines { "GRAPHICS_API_D3D11=1" }

	filter { "options:gfxapi=metal" }
		defines { "GRAPHICS_API_METAL=1" }
		files { "src/gl/metal/shaders.metallib" }
		xcodebuildsettings {
			["CLANG_ENABLE_OBJC_ARC"] = "YES",
			["GCC_ENABLE_OBJC_EXCEPTIONS"] = "YES", 
		}
		links { "MetalKit.framework", "Metal.framework" }
		prebuildcommands {
			"xcrun -sdk macosx metal -c src/gl/metal/Shaders.metal -o lib.air",
			"xcrun -sdk macosx metallib lib.air -o src/gl/metal/shaders.metallib",
			"rm lib.air",
		}

	filter { "options:gfxapi=metal", "system:macosx" }
		links { "AppKit.framework" }

	filter { "options:not gfxapi=opengl", "files:src/gl/opengl/*", "system:not macosx" }
		flags { "ExcludeFromBuild" }
	filter { "options:not gfxapi=d3d11", "files:src/gl/directx11/*", "system:not macosx" }
		flags { "ExcludeFromBuild" }
	filter { "options:not gfxapi=metal", "files:src/gl/metal/*", "system:not macosx" }
		flags { "ExcludeFromBuild" }

	filter { "system:not ios and not macosx", "files:src/**.mm" }
		flags { "ExcludeFromBuild" }

	-- include common stuff
	dofile "bin/premake/common-proj.lua"

	filter { "system:ios" }
		removeflags { "FatalWarnings" }

	filter { "not options:gfxapi=opengl" }
		objdir ("Output/intermediate/%{prj.name}/%{cfg.buildcfg}_%{cfg.platform}" .. _OPTIONS["gfxapi"])
		targetname ("%{prj.name}_" .. _OPTIONS["gfxapi"])

	filter {}
	
	targetdir "builds"
	debugdir "builds"
