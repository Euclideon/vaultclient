UDSDK_HOME = path.translate(_OPTIONS["udsdk"] or '')

project "udStreamApp"
	-- Settings
	kind "packaging"
	system "android"

	files { "../../builds/**.json", "../../builds/**.md", "../../builds/**.otf", "../../builds/**.png", "../../builds/**.jpg", "../../builds/**.dat" }
	files { "../../3rdParty/SDL2-2.0.8/lib/android/%{cfg.platform}/**.so" }
	files { "res/**.xml", "res/**.png" }
	files { "src/**.java"}

	if mergeAndroidPackage then
		files { "libs/**.so" }

		-- Create shaders folder for shaders to be copied into
		os.mkdir("../../builds/assets/shaders")
	end

	for _, file in ipairs(os.matchfiles("../../src/gl/opengl/shaders/mobile/*")) do
		local newfile = file:gsub("../../src/gl/opengl/shaders/mobile", "../../builds/assets/shaders")
		if mergeAndroidPackage then
			os.copyfile(file, newfile)
		end
		files(newfile)
	end

	vpaths {
		["libs/arm64-v8a/*"] = { "../../3rdParty/SDL2-2.0.8/lib/android/arm64/**" },
		["libs/x86_64/*"] = { "../../3rdParty/SDL2-2.0.8/lib/android/x64/**" },
		["assets/*"] = { "../../builds/**" },
		[""] = { "project.lua" },
	}

	files { "AndroidManifest.xml" }
	files { "build.xml" }
	files { "project.properties" }

	libdirs { "../../3rdParty/SDL2-2.0.8/lib/android/arm64" }
	links { "udStream", "SDL2", "main" }

	filter { "options:force-udsdk" }
		files { "../../../udSDK/buildscripts/android/src/**" }
		vpaths {
			["src/*"] = { "../../../udSDK/buildscripts/android/src/**" }
		}
	filter { "options:not force-udsdk" }
		files { "packaging.lua" }
		files { "%{UDSDK_HOME}/lib/android_arm64/src/**" }
		files { "%{UDSDK_HOME}/lib/android_arm64/libudSDK.so", "%{UDSDK_HOME}/lib/android_x64/libudSDK.so" }
		vpaths {
			["src/*"] = { "%{UDSDK_HOME}/lib/android_arm64/src/**", "src/**" },
			["libs/arm64-v8a/*"] = { "%{UDSDK_HOME}/lib/android_arm64/**.so" },
			["libs/x86_64/*"] = { "%{UDSDK_HOME}/lib/android_x64/**.so" },
		}
	filter {}

	dofile "../../3rdParty/udcore/bin/premake-bin/common-proj.lua"
