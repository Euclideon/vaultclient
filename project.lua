project "vaultClient"
	--Settings
	kind "WindowedApp"

	language "C++"
	flags { "StaticRuntime", "FatalWarnings", "MultiProcessorCompile" }

	--Files to include
	files { "src/**.cpp", "src/**.h", "src/**.c" }
	files { "3rdParty/Imgui/**.cpp", "3rdParty/Imgui/**.h" }
	files { "3rdParty/stb/**.h" }
	files { "project.lua" }
	files { "docs/**.md" }
	files { "builds/releasenotes.md" }

	removefiles { "src/gl/*/*" }

	--This project includes
	includedirs { "src" }
	includedirs { "3rdParty/Imgui" }
	includedirs { "3rdParty/stb" }
	sysincludedirs { "3rdParty/SDL2-2.0.8/include" }

	defines { "IMGUI_DISABLE_OBSOLETE_FUNCTIONS" }

	symbols "On"
	injectvaultsdkbin()

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
		sysincludedirs { "3rdParty/glew/include" }
		files { "3rdParty/glew/glew.c", "src/**.rc" }
		linkoptions( "/LARGEADDRESSAWARE" )
		libdirs { "3rdParty/SDL2-2.0.8/lib/x64" }
		links { "SDL2.lib", "SDL2main.lib", "opengl32.lib", "winmm.lib", "ws2_32" }

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
		}

	filter { "system:ios" }
		files { "iOS-Info.plist", "builds/libvaultSDK.dylib", "icons/Images.xcassets" }
		xcodebuildresources { "libvaultSDK", "Images.xcassets" }
		xcodebuildsettings { ["ASSETCATALOG_COMPILER_APPICON_NAME"] = "AppIcon" }
		removefiles { "3rdParty/glew/glew.c" }
		libdirs { "3rdParty/SDL2-2.0.8/lib/ios" }
		links { "SDL2", "AudioToolbox.framework", "QuartzCore.framework", "OpenGLES.framework", "CoreGraphics.framework", "UIKit.framework", "Foundation.framework", "CoreAudio.framework", "AVFoundation.framework", "GameController.framework", "CoreMotion.framework" }

	filter { "system:macosx or ios" }
		files { "builds/assets/fonts/NotoSansCJKjp-Regular.otf", "builds/assets/icons/EuclideonClientIcon.png", "builds/assets/icons/EuclideonLogo.png", "builds/assets/skyboxes/CloudWater_*.jpg", "builds/releasenotes.md" }
		xcodebuildresources { "NotoSans", "EuclideonClientIcon", "EuclideonLogo", "CloudWater", "releasenotes" }

	filter { "system:not windows" }
		links { "dl" }

	filter { "system:linux" }
		links { "z" }

	filter { "options:gfxapi=opengl" }
		files { "src/gl/opengl/*" }
		defines { "GRAPHICS_API_OPENGL=1" }

	filter { "options:gfxapi=d3d11" }
		files { "src/gl/directx11/*" }
		libdirs { "$(DXSDK_DIR)/Lib/x64;" }
		links { "d3d11.lib", "d3dcompiler.lib", "dxgi.lib", "dxguid.lib" }
		defines { "GRAPHICS_API_D3D11=1" }

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
