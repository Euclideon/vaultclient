-- This is used in packaging.lua to determine how to link things
mergeAndroidPackage = true

newoption {
	trigger     = "force-udsdk",
	description = "Force the use of the vaultsdk repository"
}

newoption {
	trigger     = "udSDK",
	value       = "Path",
	description = "Path to udSDK",
	default     = os.getenv("UDSDK_HOME")
}

workspace "udStreamPackage"
	configurations { "Debug", "Release" }
	platforms { "x64", "arm64" }
	system "android"
	startproject "udStreamApp"

	dofile "packaging.lua"
