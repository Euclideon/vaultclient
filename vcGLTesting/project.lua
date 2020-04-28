project "vcGLTesting"
	--Settings
	kind "ConsoleApp"

	language "C++"
	staticruntime "On"
	flags { "MultiProcessorCompile" }

	--Files to include
	files { "src/**.cpp", "src/**.h", "src/**.c", "src/**.mm" }

	--This project includes
	includedirs { "src" }
	includedirs { "../3rdParty/udcore/Include" }
	includedirs { "../3rdParty/udcore/3rdParty/stb" }
	includedirs { "../3rdParty/udcore/3rdParty/googletest/include" }
	includedirs { "../vcGL/src" }

	links { "googletest", "udCore", "vcGL" }

	-- filters
	filter { "configurations:Debug" }
		optimize "Debug"

	filter { "configurations:Debug", "system:Windows" }
		ignoredefaultlibraries { "libcmt" }

	filter { "configurations:Release" }
		optimize "Full"
		omitframepointer "On"
		flags { "NoBufferSecurityCheck" }

	filter { "system:windows" }
		sysincludedirs { "../3rdParty/SDL2-2.0.8/include" }
		linkoptions( "/LARGEADDRESSAWARE" )
		libdirs { "../3rdParty/SDL2-2.0.8/lib/x64" }
		links { "SDL2.lib", "SDL2main.lib", "winmm.lib", "ws2_32", "winhttp", "imm32.lib" }

	filter { "system:linux" }
		links { "SDL2", "GL" }
		includedirs { "../3rdParty" }
		files { "../3rdParty/GL/glext.h" }

	filter { "system:macosx" }
		frameworkdirs { "/Library/Frameworks/" }
		links { "SDL2.framework" }
		xcodebuildsettings {
			["MACOSX_DEPLOYMENT_TARGET"] = "10.13",
		}

	filter { "system:not windows" }
		links { "dl" }

	filter { "system:linux" }
		links { "z" }

	filter { "options:gfxapi=opengl" }
		defines { "GRAPHICS_API_OPENGL=1" }

	filter { "options:gfxapi=opengl", "system:macosx or ios" }
		links { "OpenGL.framework" }

	filter { "options:gfxapi=opengl", "system:windows" }
		links { "opengl32.lib" }

	filter { "options:gfxapi=d3d11" }
		libdirs { "$(DXSDK_DIR)/Lib/x64;" }
		links { "d3d11.lib", "d3dcompiler.lib", "dxgi.lib", "dxguid.lib" }
		defines { "GRAPHICS_API_D3D11=1" }

	filter { "options:gfxapi=metal" }
		defines { "GRAPHICS_API_METAL=1" }
		xcodebuildsettings {
			["CLANG_ENABLE_OBJC_ARC"] = "YES",
			["GCC_ENABLE_OBJC_EXCEPTIONS"] = "YES",
		}
		links { "MetalKit.framework", "Metal.framework" }

	filter { "options:gfxapi=metal", "system:macosx" }
		links { "AppKit.framework", "QuartzCore.framework" }

	filter { "system:not ios", "system:not macosx", "files:src/**.mm" }
		flags { "ExcludeFromBuild" }

	-- include common stuff
	dofile "../3rdParty/udcore/bin/premake-bin/common-proj.lua"

	floatingpoint "default"

	filter { "system:ios or system:macosx" }
		removeflags { "FatalWarnings" }

	filter { "files:../3rdParty/**" }
		warnings "Off"

	filter { "not options:gfxapi=opengl" }
		objdir ("Output/intermediate/%{prj.name}/%{cfg.buildcfg}_%{cfg.platform}" .. _OPTIONS["gfxapi"])
		targetname ("%{prj.name}_" .. _OPTIONS["gfxapi"])

	filter {}

	symbols "On"
	targetdir "../builds"
	debugdir "../builds"
