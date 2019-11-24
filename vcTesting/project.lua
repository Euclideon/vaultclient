project "vcTesting"
	kind "ConsoleApp"
	language "C++"
	staticruntime "On" 
	tags { "vault-project" }

	files { "src/**.cpp", "src/**.h" }
	files { "project.lua" }

	includedirs { "src", "../3rdParty/udcore/include", "../3rdParty/udcore/3rdParty/googletest/include", "../src" }

	-- files to test
	files { "../src/vcUnitConversion.*" }


	-- Linking
	links { "googletest", "udCore" }

	filter { "system:windows" }
		links { "iphlpapi.lib", "winmm.lib", "ws2_32", "dbghelp.lib", "wldap32.lib" }
		flags { "FatalWarnings" }

	filter { "system:linux" }
		links { "z", "dl" }
		libdirs { "../builds/tests" }

	filter { }

	-- include common stuff
	dofile "../3rdParty/udcore/bin/premake-bin/common-proj.lua"
	floatingpoint "strict"

	-- we use exceptions
	exceptionhandling "Default"

	filter { "options:coverage" }
		buildoptions { "-fprofile-arcs", "-ftest-coverage" }
		linkoptions { "-fprofile-arcs" }
		optimize "Off"
	filter {  }

	targetdir "../builds/tests"
	debugdir "../"
