# Make folder to store the app to build a DMG from
mkdir -p builds/packaging/.background

# Copy respective app
if [ $1 == "metal" ]; then
	cp -af builds/vaultClient_metal.app builds/packaging/vaultClient_metal.app
	export APPNAME=vaultClient_metal
else
	cp -af builds/vaultClient.app builds/packaging/vaultClient.app
	export APPNAME=vaultClient
fi
cp icons/dmgBackground.png builds/packaging/.background/background.png

# See https://stackoverflow.com/a/1513578 for reference
hdiutil create builds/vaultClient.temp.dmg -volname "vaultClient" -srcfolder builds/packaging -format UDRW -fs HFS+ -fsargs "-c c=64,a=16,e=16"
device=$(hdiutil attach -readwrite -noverify -noautoopen "builds/vaultClient.temp.dmg" | egrep '^/dev/' | sed 1q | awk '{print $1}')

# Delay 10 to give the disk time to mount - otherwise builds fail
echo '
	delay 10
	tell application "Finder"
		tell disk "vaultClient"
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
chmod -Rf go-w /Volumes/vaultClient
cp icons/macOSAppIcons.icns /Volumes/vaultClient/.VolumeIcon.icns
SetFile -c icnC "/Volumes/vaultClient/.VolumeIcon.icns"
SetFile -a C /Volumes/vaultClient
sync
sync
hdiutil unmount /Volumes/vaultClient -force
hdiutil detach ${device} -force
hdiutil convert "builds/vaultClient.temp.dmg" -format UDZO -imagekey zlib-level=9 -o "builds/vaultClient.dmg"
rm -f builds/vaultClient.temp.dmg
rm -rf builds/packaging
