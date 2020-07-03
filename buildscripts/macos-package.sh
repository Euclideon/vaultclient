function CreateDMG {
	# Make folder to store the app to build a DMG from
	mkdir -p builds/packaging/.background

	# Create app
	mkdir builds/packaging/udStream.app
	export APPNAME=udStream

	# Copy background file
	cp icons/dmgBackground.png builds/packaging/.background/background.png

	# See https://stackoverflow.com/a/1513578 for reference
	hdiutil create builds/udStream.temp.dmg -volname "udStream" -srcfolder builds/packaging -format UDRW -fs HFS+ -fsargs "-c c=64,a=16,e=16"
	device=$(hdiutil attach -readwrite -noverify -noautoopen "builds/udStream.temp.dmg" | egrep '^/dev/' | sed 1q | awk '{print $1}')

	# Delay 10 to give the disk time to mount - otherwise builds fail
	echo '
		delay 10
		tell application "Finder"
			tell disk "udStream"
				open
				set current view of container window to icon view
				set toolbar visible of container window to false
				set statusbar visible of container window to false
				set the bounds of container window to {400, 100, (400 + 512), (100 + 320 + 23)}
				set theViewOptions to the icon view options of container window
				set arrangement of theViewOptions to not arranged
				set icon size of theViewOptions to 72
				set background picture of theViewOptions to file ".background:background.png"
				make new alias file at container window to POSIX file "/Applications" with properties {name:"Applications"}
				set position of item "APPNAME" of container window to {150, 180}
				set position of item "Applications" of container window to {360, 180}
				update without registering applications
				delay 5
				close
			end tell
		end tell
	' | sed "s/APPNAME/$APPNAME/" | osascript

	# Change permissions
	chmod -Rf go-w /Volumes/udStream

	# Copy icon and set it as the DMG icon
	cp icons/macOSAppIcons.icns /Volumes/udStream/.VolumeIcon.icns
	SetFile -c icnC "/Volumes/udStream/.VolumeIcon.icns"
	SetFile -a C /Volumes/udStream

	# Sync changes - people have reported that it needs to be done twice
	sync
	sync

	# Unmount and detach the volume, either should work but seem to fail on their own sometimes
	hdiutil unmount /Volumes/udStream -force
	hdiutil detach ${device} -force

	# Cleanup packaing folder
	rm -rf builds/packaging
}

function UpdateDMG {
	# Resize the DMG
	hdiutil resize -size 100m builds/udStream.temp.dmg

	# Mount the DMG
	device=$(hdiutil attach -readwrite -noverify -noautoopen "builds/udStream.temp.dmg" | egrep '^/dev/' | sed 1q | awk '{print $1}')

	# Change permissions
	chmod -Rf go-w /Volumes/udStream

	# Update the build
	cp -af builds/udStream.app /Volumes/udStream/

	# Sync changes - people have reported that it needs to be done twice
	sync
	sync

	# Unmount and detach the volume, either should work but seem to fail on their own sometimes
	hdiutil unmount /Volumes/udStream -force
	hdiutil detach ${device} -force

	# Convert DMG to compressed and read-only format
	hdiutil convert "builds/udStream.temp.dmg" -format UDZO -imagekey zlib-level=9 -o "builds/udStream.dmg"

	# Cleanup temporary DMG
	rm -f builds/udStream.temp.dmg
}

if [ "$1" == "create" ]; then
	CreateDMG
else
	UpdateDMG
fi
