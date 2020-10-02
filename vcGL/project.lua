project "vcGL"
	--Settings
	kind "StaticLib"

	language "C++"
	staticruntime "On"
	flags { "FatalWarnings", "MultiProcessorCompile" }

	--Files to include
	files { "src/**.cpp", "src/**.h", "src/**.c", "src/**.mm" }
	files { "src/**.hlsl", "src/**.vert", "src/**.frag", "src/**.metal" }
	files { "../3rdParty/stb/**.h" }
	files { "project.lua" }

	--This project includes
	includedirs { "src" }
	includedirs { "../3rdParty/udcore/Include" }
	includedirs { "../3rdParty/udcore/3rdParty/stb" }
	includedirs { "../3rdParty/libtiff/libtiff" }

	links { "libtiff" }

	local excludedSourceFileNames = {}
	if _OPTIONS["gfxapi"] ~= "opengl" then
		table.insert(excludedSourceFileNames, "src/opengl/*");
	end
	if _OPTIONS["gfxapi"] ~= "d3d11" then
		table.insert(excludedSourceFileNames, "src/directx11/*");
	end
	if _OPTIONS["gfxapi"] ~= "metal" then
		table.insert(excludedSourceFileNames, "src/metal/*");
	end

	-- filters
	filter { "configurations:Debug" }
		optimize "Debug"
		removeflags { "FatalWarnings" }

	filter { "configurations:Debug", "system:Windows" }
		ignoredefaultlibraries { "libcmt" }

	filter { "configurations:Release" }
		optimize "Full"
		omitframepointer "On"
		flags { "NoBufferSecurityCheck" }

	filter { "system:windows" }
		sysincludedirs { "../3rdParty/SDL2-2.0.8/include" }

	filter { "system:linux" }
		includedirs { "../3rdParty" }
		files { "../3rdParty/GL/glext.h" }

	filter { "system:android" }
		sysincludedirs { "../3rdParty/SDL2-2.0.8/include" }
		includedirs { "../3rdParty" }

	filter { "system:macosx" }
		frameworkdirs { "/Library/Frameworks/" }
		links { "SDL2.framework" }
		xcodebuildsettings {
			["MACOSX_DEPLOYMENT_TARGET"] = "10.13",
		}

	filter { "system:ios" }
		sysincludedirs { "../3rdParty/SDL2-2.0.8/include" }

	filter { "system:macosx or ios" }
		xcodebuildsettings { ["EXCLUDED_SOURCE_FILE_NAMES"] = excludedSourceFileNames }

	filter { "system:emscripten" }
		linkoptions  { "-s USE_WEBGL2=1", "-s FULL_ES3=1", --[["-s DEMANGLE_SUPPORT=1",]] "-s EXTRA_EXPORTED_RUNTIME_METHODS='[\"ccall\", \"cwrap\", \"getValue\", \"setValue\", \"UTF8ToString\", \"stringToUTF8\"]'" }

	filter { "system:emscripten", "options:force-vaultsdk" }
		removeincludedirs { "../3rdParty/udcore/Include" }
		includedirs { "../../vault/ud/udCore/Include" }

	filter { "options:gfxapi=opengl" }
		defines { "GRAPHICS_API_OPENGL=1" }

	filter { "options:gfxapi=opengl", "system:windows" }
		defines { "GLEW_STATIC" }
		sysincludedirs { "../3rdParty/glew/include" }
		files { "../3rdParty/glew/glew.c" }

	filter { "options:gfxapi=opengl", "system:macosx" }
		links { "OpenGL.framework" }

	filter { "options:gfxapi=d3d11" }
		defines { "GRAPHICS_API_D3D11=1" }

	filter { "options:gfxapi=metal" }
		defines { "GRAPHICS_API_METAL=1" }
		xcodebuildsettings {
			["CLANG_ENABLE_OBJC_ARC"] = "YES",
			["GCC_ENABLE_OBJC_EXCEPTIONS"] = "YES",
		}

	filter { "options:not gfxapi=opengl", "files:src/opengl/*", "system:not macosx" }
		flags { "ExcludeFromBuild" }
	filter { "options:not gfxapi=d3d11", "files:src/directx11/*", "system:not macosx" }
		flags { "ExcludeFromBuild" }
	filter { "options:not gfxapi=metal", "files:src/metal/*", "system:not macosx" }
		flags { "ExcludeFromBuild" }

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
