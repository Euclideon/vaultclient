project "udStreamConvertCMD"
	--Settings
	kind "ConsoleApp"

	language "C++"
	staticruntime "On"

	--Files to include
	files { "src/**.cpp", "src/**.h" }
	files { "project.lua" }

	--This project includes
	includedirs { "src" }
	includedirs { "../3rdParty/udcore/Include" }

	links { "udCore" .. (projectSuffix or "") }
	injectvaultsdkbin()

	filter { "system:windows" }
		links { "ws2_32.lib", "winmm.lib" }
		files { "**.rc" }

	filter { "system:macosx" }
		frameworkdirs { "/Library/Frameworks/" }
		links { "CoreFoundation.framework", "Security.framework" }

	filter { "system:linux" }
		linkoptions { "-Wl,-rpath '-Wl,$$ORIGIN'" } -- Check beside the executable for the SDK

	filter {}

	-- include common stuff
	dofile "../3rdParty/udcore/bin/premake-bin/common-proj.lua"

	filter {}

	targetdir "../builds"
	debugdir "../builds"

