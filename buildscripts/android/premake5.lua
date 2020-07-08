-- This is used in packaging.lua to determine how to link things
mergeAndroidPackage = true

newoption {
	trigger     = "force-vaultsdk",
	description = "Force the use of the vaultsdk repository"
}

newoption {
	trigger     = "vaultsdk",
	value       = "Path",
	description = "Path to Vault SDK",
	default     = os.getenv("VAULTSDK_HOME")
}

workspace "udStreamPackage"
	configurations { "Debug", "Release" }
	platforms { "x64", "arm64" }
	system "android"
	startproject "udStreamApp"

	dofile "packaging.lua"
