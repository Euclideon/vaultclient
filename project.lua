project "vaultClient"
	--Settings
	kind "ConsoleApp"
	filter { "system:macosx OR ios OR android" }
		kind "WindowedApp"
	filter {}

	language "C++"
	flags { "StaticRuntime", "FatalWarnings", "MultiProcessorCompile" }

	--Files to include
	files { "src/**.cpp", "src/**.h", "src/**.c" }
	files { "3rdParty/Imgui/**.cpp", "3rdParty/Imgui/**.h" }
	files { "project.lua" }

	--This project includes
	includedirs { "src" }
	includedirs { "3rdParty/Imgui" }
	includedirs { "3rdParty/stb" }
	sysincludedirs { "3rdParty/SDL2-2.0.5/include" }

	defines { "IMGUI_DISABLE_OBSOLETE_FUNCTIONS" }

	symbols "On"
	injectvaultsdkbin()

	-- filters
	filter { "configurations:Debug" }
		optimize "Debug"
		removeflags { "FatalWarnings" }

	filter { "configurations:Debug", "system:Windows" }
		ignoredefaultlibraries { "libcmt" }

	filter { "configurations:Release" }
		optimize "Full"
		flags { "NoFramePointer", "NoBufferSecurityCheck" }

	filter { "system:linux or windows" }
		defines { "GLEW_STATIC" }
		sysincludedirs { "3rdParty/glew/include" }
		files { "3rdParty/glew/glew.c" }

	-- Turn off warnings as errors for ImGui.
	filter { "system:linux" }
		removeflags { "FatalWarnings" }

	filter { "system:windows" }
		linkoptions( "/LARGEADDRESSAWARE" )
		libdirs { "3rdParty/SDL2-2.0.5/lib/x64" }
		links { "SDL2.lib", "opengl32.lib", "winmm.lib" }

	filter { "system:linux" }
		linkoptions { "-Wl,-rpath '-Wl,$$ORIGIN'" } -- Check beside the executable for the SDK
		links { "SDL2", "GL" }

	filter { "system:macosx" }
		frameworkdirs { "/Library/Frameworks/" }
		links { "SDL2.framework", "OpenGL.framework" }

	filter { "system:ios" }
		files { "Info.plist", "builds/client/bin/libvaultSDK.dylib" }
		xcodebuildresources { "libvaultSDK" }
		removefiles { "3rdParty/glew/glew.c" }
		libdirs { "3rdParty/SDL2-2.0.5/lib/ios" }
		links { "SDL2", "AudioToolbox.framework", "QuartzCore.framework", "OpenGLES.framework", "CoreGraphics.framework", "UIKit.framework", "Foundation.framework", "CoreAudio.framework", "AVFoundation.framework", "GameController.framework", "CoreMotion.framework" }

	filter { "system:not windows" }
		links { "dl" }

	filter { "system:linux" }
		links { "z" }

	filter {}

	-- include common stuff
	dofile "bin/premake/common-proj.lua"

	filter { "system:ios" }
		removeflags { "FatalWarnings" }
	filter {}

	targetdir "builds/client/bin"
	debugdir "builds/client/bin"
