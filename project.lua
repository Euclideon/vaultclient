project "vaultClient"
	--Settings
	filter { "system:not ios", "system:not android" }
		kind "ConsoleApp"
	filter { "system:ios OR android" }
		kind "WindowedApp"
	filter {}
	
	language "C++"
	flags { "StaticRuntime", "FatalWarnings", "MultiProcessorCompile" }

	--Files to include
	files { "src/**.cpp", "src/**.h" }
	files { "project.lua" }

	--This project includes
	includedirs { "src" }

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

	filter { "system:windows" }
		linkoptions( "/LARGEADDRESSAWARE" )

	filter { "system:not windows" }
		links { "dl" }
		
	filter { "system:linux" }
		links { "z" }

	filter {}

	-- include common stuff
	dofile "bin/premake/common-proj.lua"
	floatingpoint "strict"

	targetdir "builds/client/bin"
	debugdir "builds/client/bin"
