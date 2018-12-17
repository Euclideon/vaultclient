project "vaultConvertCMD"
	--Settings
	kind "ConsoleApp"

	language "C++"
	flags { "StaticRuntime" }

	--Files to include
	files { "src/**.cpp", "src/**.h" }
	files { "../src/udPlatform/**.cpp", "../src/udPlatform/**.h" }
	files { "project.lua" }

	--This project includes
	includedirs { "src" }
	includedirs { "../src/udPlatform" }

	injectvaultsdkbin()

	filter { "system:windows" }
		links { "ws2_32.lib", "winmm.lib" }
		files { "**.rc" }

	filter { "system:macosx" }
		frameworkdirs { "/Library/Frameworks/" }
		links { "CoreFoundation.framework", "Security.framework" }
	filter {}

	-- include common stuff
	dofile "../bin/premake/common-proj.lua"

	filter {}

	targetdir "../builds"
	debugdir "../builds"

