#!/bin/bash
ARCHPREFIX=()
arch -arm64 /usr/bin/true 2>/dev/null && ARCHPREFIX=(/usr/bin/arch -arm64)
"${ARCHPREFIX[@]}" /Applications/MEGAcmd.app/Contents/MacOS/mega-cmd &
sleep 2
osascript -e 'tell app "Terminal" to do script "/Applications/MEGAcmd.app/Contents/MacOS/MEGAcmdShell;exit" activate'
