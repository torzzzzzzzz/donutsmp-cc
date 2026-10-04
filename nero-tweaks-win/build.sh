#!/bin/sh
# Cross-compile NeroTweaks-FreeVersion.exe on Linux (needs: apt install mingw-w64)
set -e
cd "$(dirname "$0")"
x86_64-w64-mingw32-windres nero.rc -O coff -o nero_res.o
x86_64-w64-mingw32-gcc -O2 -Wall -municode -mwindows -o NeroTweaks-FreeVersion.exe main.c nero_res.o \
  -lgdi32 -lmsimg32 -liphlpapi -lpowrprof -lshell32 -lole32 -luser32 -lbcrypt -static -s
rm -f nero_res.o
echo built NeroTweaks-FreeVersion.exe
