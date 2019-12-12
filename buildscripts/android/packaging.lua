project "EuclideonVaultClient"
  -- Settings
  kind "packaging"
  system "android"

  files { "../../builds/**.json", "../../builds/**.md", "../../builds/**.otf", "../../builds/**.png", "../../builds/**.jpg" }
  files { "../../3rdParty/SDL2-2.0.8/lib/android/%{cfg.platform}/**.so" }
  files { "res/**.xml", "res/**.png" }
  files { "src/**.java"}
  
  vpaths {
		["libs/arm64-v8a/*"] = { "../../3rdParty/SDL2-2.0.8/lib/android/arm64/**" },
		["libs/x64-v8a/*"] = { "../../3rdParty/SDL2-2.0.8/lib/android/x64/**" },
		["assets/*"] = { "../../builds/**" },
		[""] = { "project.lua" },
	}

  files { "AndroidManifest.xml" }
  files { "build.xml" }
  files { "project.properties" }

  libdirs { "../../3rdParty/SDL2-2.0.8/lib/android/arm64" }
  links { "vaultClient", "SDL2", "main" }

  dofile "../../3rdParty/udcore/bin/premake-bin/common-proj.lua"
