function injectudbin()
	-- Calculate the paths
	local ud2Location = path.getrelative(_SCRIPT_DIR, _MAIN_SCRIPT_DIR .. "/../vault/ud")
	local osname = "windows"
	local distroExtension = ""
	if os.get() == premake.MACOSX then
		osname = iif(_OPTIONS["ios"], "ios", "macosx")
	elseif os.get() ~= premake.WINDOWS then
		osname = os.outputof('lsb_release -ir | head -n2 | cut -d ":" -f 2 | tr -d "\n\t" | tr [:upper:] [:lower:] | cut -d "." -f 1')
		distroExtension = iif(string.startswith(osname, "ubuntu"), "_deb", "_rpm")
	end

	-- Calculate the libdir location
	local libPath = "/lib"
	local system = "/%{cfg.system}"
	local shortname = iif(osname == "windows", "_%{cfg.shortname}", "_%{cfg.shortname:gsub('clang', '')}")
	local compilerExtension = iif(osname == "windows", "", "_%{cfg.toolset or 'gcc'}")

	local ud2Libdir = ud2Location .. libPath .. system .. shortname .. compilerExtension .. distroExtension

	-- Call the Premake APIs
	links { "udPointCloud", "udPlatform" }
	includedirs { ud2Location .. "/udPlatform/Include", ud2Location .. "/udPointCloud/Include" }
	libdirs { ud2Libdir }
end


function injectvaultsdkbin()
	-- Calculate the paths
	local osname = "windows"
	local distroExtension = ""
	if os.get() == premake.MACOSX then
		osname = iif(_OPTIONS["ios"], "ios", "macosx")
	elseif os.get() ~= premake.WINDOWS then
		osname = os.outputof('lsb_release -ir | head -n2 | cut -d ":" -f 2 | tr -d "\n\t" | tr [:upper:] [:lower:] | cut -d "." -f 1')
		distroExtension = iif(string.startswith(osname, "ubuntu"), "_deb", "_rpm")
	end

	if(os.get() == premake.MACOSX) then
		links { "vaultSDK.framework" }
	else
		links { "vaultSDK" }
	end
  
	if(_OPTIONS["force-vaultsdk"]) then
		includedirs { "../vault/vaultsdk/src" }
	else
		if(os.getenv("VAULTSDK_HOME") == nil) then
			error "VaultSDK not installed correctly. (No VAULTSDK_HOME environment variable set!)"
		end

		if(os.get() == premake.WINDOWS) then
			os.execute('Robocopy "%VAULTSDK_HOME%/Include" "Include" /s /purge')
			os.copyfile(os.getenv("VAULTSDK_HOME") .. "/lib/win_x64/vaultSDK.dll", "builds/client/bin/vaultSDK.dll")
			os.copyfile(os.getenv("VAULTSDK_HOME") .. "/lib/win_x64/vaultSDK.lib", "src/vaultSDK.lib")
			libdirs { "src" }
		elseif(os.get() == premake.MACOSX) then
			os.execute("mkdir -p builds/client/bin")

			-- copy dmg, mount, extract framework, unmount then remove.
			os.copyfile(os.getenv("VAULTSDK_HOME") .. "/lib/osx_x64/vaultSDK.dmg", "builds/client/bin/vaultSDK.dmg")
			os.execute("/usr/bin/hdiutil attach builds/client/bin/vaultSDK.dmg")
			os.execute("cp -a -f /Volumes/vaultSDK/vaultSDK.framework builds/client/bin/")
			os.execute("/usr/bin/hdiutil detach /Volumes/vaultSDK")
			os.execute("rm -r builds/client/bin/vaultSDK.dmg")

			os.execute("cp -R " .. os.getenv("VAULTSDK_HOME") .. "/Include .")
			files { "builds/client/bin/vaultSDK.framework" }
			libdirs { "builds/client/bin" }
		else
			os.execute("mkdir -p builds/client/bin")
			os.copyfile(os.getenv("VAULTSDK_HOME") .. "/lib/linux_GCC_x64/libvaultSDK.so", "builds/client/bin/libvaultSDK.so")
			os.execute("cp -R " .. os.getenv("VAULTSDK_HOME") .. "/Include .")
			libdirs { "builds/client/bin" }
		end

		includedirs { "Include" }
	end
end

newoption {
	trigger     = "force-vaultsdk",
	description = "Force the use of the vaultsdk repository"
}

-- Fix bug in Clang toolset
if premake.tools.clang.cflags.floatingpoint ~= nil then
	premake.tools.clang.cflags.floatingpoint = { Fast = "-ffast-math", }
end

solution "vaultClient"
	-- This hack just makes the VS project and also the makefile output their configurations in the idiomatic order
	if _ACTION == "gmake" and _OS == "linux" then
		configurations { "Release", "Debug", "ReleaseClang", "DebugClang" }
		linkgroups "On"
		filter { "configurations:*Clang" }
			toolset "clang"
		filter { }
	elseif _OS == "macosx" then
		configurations { "Release", "Debug" }
		toolset "clang"
	else
		configurations { "Debug", "Release" }
	end

	platforms { "x64" }
	editorintegration "on"

	flags { "C++11" }
	pic "On"

	xcodebuildsettings { ["CLANG_CXX_LANGUAGE_STANDARD"] = "c++0x" }

	-- Strings
	if os.getenv("CI_BUILD_REF_NAME") then
		defines { "GIT_BRANCH=\"" .. os.getenv("CI_BUILD_REF_NAME") .. "\"" }
	end
	if os.getenv("CI_BUILD_REF") then
		defines { "GIT_REF=\"" .. os.getenv("CI_BUILD_REF") .. "\"" }
	end

	-- Numbers
	if os.getenv("CI_PIPELINE_ID") then
		defines { "GIT_BUILD=" .. os.getenv("CI_PIPELINE_ID") }
	end
	if os.getenv("UNIXTIME") then
		defines { "BUILD_TIME=" .. os.getenv("UNIXTIME") }
	end

	if _OPTIONS["force-vaultsdk"] then
		if os.get() ~= premake.MACOSX then
			dofile "../vault/3rdParty/curl/project.lua"
		end
		dofile "../vault/vaultsdk/project.lua"
		xcodebuildsettings { 
			['INSTALL_PATH'] = "@executable_path/../Frameworks",
			['SKIP_INSTALL'] = "YES"
		}
		targetdir "builds/client/bin"
		debugdir "builds/client/bin" 
	end

	dofile "project.lua"
