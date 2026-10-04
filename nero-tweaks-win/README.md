# Nero Tweaks (native Windows app, C)

`NeroTweaks.exe` is a single file - double-click it (it asks for administrator rights, which tweaks need).

Rebuild on Linux: `sudo apt install mingw-w64 && ./build.sh`

All 15 tweaks are real and reversible (original values are saved and restored):
power plan, Game Mode, Game DVR, Game Bar, fullscreen optimizations, power throttling,
game CPU scheduling, network throttling, Nagle's algorithm, update sharing, mouse acceleration,
sticky keys popup, Fortnite CPU priority, background apps, hibernation.

Game Library edits Fortnite's GameUserSettings.ini (backed up, restored when turned off).
Addons: performance overlay, shader cache cleaner, background app closer, temp cleaner, DNS flush.

`NeroTweaks.exe --selftest` applies and reverts every tweak and writes selftest.log.
