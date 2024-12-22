#!/bin/bash
/Applications/MEGAcmd.app/Contents/MacOS/mega-cmd &
sleep 1
osascript -e 'tell app "Terminal" to do script "/Applications/MEGAcmd.app/Contents/MacOS/MEGAcmdShell;exit" activate'
