#!/bin/sh
# Cross-compile on Linux (needs: apt install mingw-w64).
#   ./build.sh          -> NeroTweaks-FreeVersion.exe (the public free/paid build)
#   ./build.sh owner    -> NeroTweaks-OwnerVersion.exe (everything unlocked + code maker; PRIVATE, never publish)
set -e
cd "$(dirname "$0")"
x86_64-w64-mingw32-windres nero.rc -O coff -o nero_res.o
LIBS="-lgdi32 -lmsimg32 -liphlpapi -lpowrprof -lshell32 -lole32 -luser32 -lbcrypt -lurlmon -lgdiplus -luuid -lwininet"
if [ "$1" = "owner" ]; then
  x86_64-w64-mingw32-gcc -O2 -Wall -municode -mwindows -DOWNER_BUILD -o NeroTweaks-OwnerVersion.exe main.c nero_res.o $LIBS -static -s
  echo built NeroTweaks-OwnerVersion.exe
else
  x86_64-w64-mingw32-gcc -O2 -Wall -municode -mwindows -o NeroTweaks-FreeVersion.exe main.c nero_res.o $LIBS -static -s
  echo built NeroTweaks-FreeVersion.exe
fi
rm -f nero_res.o
