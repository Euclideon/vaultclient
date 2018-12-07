require "ios"

table.insert(premake.option.get("os").allowed, { "ios", "Apple iOS" })

filter { "system:ios", "action:xcode4" }
  xcodebuildsettings {
    ['PRODUCT_BUNDLE_IDENTIFIER'] = 'Euclideon Client',
    ["CODE_SIGN_IDENTITY[sdk=iphoneos*]"] = "iPhone Developer",
    ['IPHONEOS_DEPLOYMENT_TARGET'] = '10.3',
    ['SDKROOT'] = 'iphoneos',
    ['ARCHS'] = 'arm64',
    ['TARGETED_DEVICE_FAMILY'] = "1,2",
    ['DEVELOPMENT_TEAM'] = "452P989JPT",
    ['ENABLE_BITCODE'] = "NO",
  }

function getosinfo()
	local osname = "windows"
	local distroExtension = ""
	if os.target() == premake.MACOSX then
		osname = "macosx"
	elseif os.target() == premake.IOS then
		osname = "ios"
	elseif os.target() ~= premake.WINDOWS then
		osname = os.outputof('lsb_release -ir | head -n2 | cut -d ":" -f 2 | tr -d "\n\t" | tr [:upper:] [:lower:] | cut -d "." -f 1')
		distroExtension = iif(string.startswith(osname, "ubuntu"), "_deb", "_rpm")
	end

	return osname, distroExtension
end

function injectudbin()
	-- Calculate the paths
	local ud2Location = path.getrelative(_SCRIPT_DIR, _MAIN_SCRIPT_DIR .. "/../vault/ud")
	local osname, distroExtension = getosinfo()

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
	local osname, distroExtension = getosinfo()

	if os.target() == premake.MACOSX then
		links { "vaultSDK.framework" }
	else
		links { "vaultSDK" }
	end

	if _OPTIONS["force-vaultsdk"] then
		includedirs { "%{wks.location}/../vault/vaultsdk/src" }
	else
		if os.getenv("VAULTSDK_HOME") == nil then
			error "VaultSDK not installed correctly. (No VAULTSDK_HOME environment variable set!)"
		end

		if os.target() == premake.WINDOWS then
			os.execute('Robocopy "%VAULTSDK_HOME%/Include" "Include" /s /purge')
			os.copyfile(os.getenv("VAULTSDK_HOME") .. "/lib/win_x64/vaultSDK.dll", "builds/vaultSDK.dll")
			os.copyfile(os.getenv("VAULTSDK_HOME") .. "/lib/win_x64/vaultSDK.lib", "src/vaultSDK.lib")
			libdirs { "src" }
		elseif os.target() == premake.MACOSX then
			os.execute("mkdir -p builds")

			-- copy dmg, mount, extract framework, unmount then remove.
			os.copyfile(os.getenv("VAULTSDK_HOME") .. "/lib/osx_x64/vaultSDK.dmg", "builds/vaultSDK.dmg")
			local device = os.outputof("/usr/bin/hdiutil attach -noverify -noautoopen builds/vaultSDK.dmg | egrep '^/dev/' | sed 1q | awk '{print $1}'")
			os.execute("cp -a -f /Volumes/vaultSDK/vaultSDK.framework builds/")
			os.execute("/usr/bin/hdiutil detach /Volumes/vaultSDK")
			os.execute("/usr/bin/hdiutil detach " .. device)
			os.execute("rm -r builds/vaultSDK.dmg")

			os.execute("cp -R " .. os.getenv("VAULTSDK_HOME") .. "/Include .")
			prelinkcommands {
				"rm -rf %{prj.targetdir}/%{prj.targetname}.app/Contents/Frameworks",
				"mkdir -p %{prj.targetdir}/%{prj.targetname}.app/Contents/Frameworks",
				"cp -af builds/vaultSDK.framework %{prj.targetdir}/%{prj.targetname}.app/Contents/Frameworks/",
				"cp -af /Library/Frameworks/SDL2.framework %{prj.targetdir}/%{prj.targetname}.app/Contents/Frameworks/",
			}
			linkoptions { "-rpath @executable_path/../Frameworks/" }
			frameworkdirs { "builds" }
		elseif os.target() == premake.IOS then
			os.execute("mkdir -p builds")
			os.execute("lipo -create " .. os.getenv("VAULTSDK_HOME") .. "/lib/ios_arm64/libvaultSDK.dylib " .. os.getenv("VAULTSDK_HOME") .. "/lib/ios_x64/libvaultSDK.dylib -output builds/libvaultSDK.dylib")
			os.execute("codesign -s T6Q3JCVW77 builds/libvaultSDK.dylib") -- Is this required? Should this move to VaultSDK?
			os.execute("cp -R " .. os.getenv("VAULTSDK_HOME") .. "/Include .")
			libdirs { "builds" }
			linkoptions { "-rpath @executable_path/" }
		else
			os.execute("mkdir -p builds")
			os.copyfile(os.getenv("VAULTSDK_HOME") .. "/lib/linux_GCC_x64/libvaultSDK.so", "builds/libvaultSDK.so")
			os.execute("cp -R " .. os.getenv("VAULTSDK_HOME") .. "/Include .")
			libdirs { "builds" }
		end

		includedirs { "Include" }
	end
end

newoption {
	trigger     = "force-vaultsdk",
	description = "Force the use of the vaultsdk repository"
}

newoption {
   trigger     = "gfxapi",
   value       = "API",
   description = "Choose a particular 3D API for rendering",
   default     = "opengl",
   allowed = {
      { "opengl", "OpenGL" },
      { "d3d11", "Direct3D 11 (Windows only)" }
   }
}

solution "vaultClient"
	-- This hack just makes the VS project and also the makefile output their configurations in the idiomatic order
	if _ACTION == "gmake" and os.target() == "linux" then
		configurations { "Release", "Debug", "ReleaseClang", "DebugClang" }
		linkgroups "On"
		filter { "configurations:*Clang" }
			toolset "clang"
		filter { }
	elseif os.target() == "macosx" or os.target() == "ios" then
		configurations { "Release", "Debug" }
		toolset "clang"
	else
		configurations { "Debug", "Release" }
	end

	platforms { "x64" }
	editorintegration "on"
	startproject "vaultClient"
	cppdialect "C++11"
	pic "On"

	xcodebuildsettings { ["CLANG_CXX_LANGUAGE_STANDARD"] = "c++0x" }

	if os.target() == premake.WINDOWS then
		os.copyfile("bin/sdl/SDL2.dll", "builds/SDL2.dll")
	end

	-- Strings
	if os.getenv("CI_BUILD_REF_NAME") then
		defines { "GIT_BRANCH=\"" .. os.getenv("CI_BUILD_REF_NAME") .. "\"" }
	end
	if os.getenv("CI_BUILD_REF") then
		defines { "GIT_REF=\"" .. os.getenv("CI_BUILD_REF") .. "\"" }
	end

	-- Numbers
	if os.getenv("CI_COMMIT_TAG") then
		defines { "GIT_TAG=" .. os.getenv("CI_PIPELINE_ID") }
	end
	if os.getenv("CI_PIPELINE_ID") then
		defines { "GIT_BUILD=" .. os.getenv("CI_PIPELINE_ID") }
		xcodebuildsettings { ["INFOPLIST_PREPROCESSOR_DEFINITIONS"] = "GIT_BUILD=" .. os.getenv("CI_PIPELINE_ID") }
	end
	if os.getenv("UNIXTIME") then
		defines { "BUILD_TIME=" .. os.getenv("UNIXTIME") }
	end

	if _OPTIONS["force-vaultsdk"] then
		if os.target() ~= premake.MACOSX then
			dofile "../vault/3rdParty/curl/project.lua"
		end
		dofile "../vault/ud/udPlatform/project.lua"
		dofile "../vault/ud/udPointCloud/project.lua"
		dofile "../vault/vaultcore/project.lua"
		dofile "../vault/vaultsdk/project.lua"

		filter { "system:macosx" }
			xcodebuildsettings {
				['INSTALL_PATH'] = "@executable_path/../Frameworks",
				['SKIP_INSTALL'] = "YES"
			}
		filter {}
		targetdir "%{wks.location}/builds"
		debugdir "%{wks.location}/builds"
	end

	if os.target() ~= premake.IOS and os.target() ~= premake.ANDROID then
		dofile "vcConvertCMD/project.lua"
	end

	dofile "project.lua"
