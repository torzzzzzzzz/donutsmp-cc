# Nero Tweaks (native Windows app, C)

`NeroTweaks-FreeVersion.exe` is a single file - double-click it (it asks for administrator rights, which tweaks need).

Rebuild on Linux: `sudo apt install mingw-w64 && ./build.sh`

All 15 tweaks are real and reversible (original values are saved and restored):
power plan, Game Mode, Game DVR, Game Bar, fullscreen optimizations, power throttling,
game CPU scheduling, network throttling, Nagle's algorithm, update sharing, mouse acceleration,
sticky keys popup, Fortnite CPU priority, background apps, hibernation.

Game Library edits Fortnite's GameUserSettings.ini (backed up, restored when turned off).
Addons: performance overlay, shader cache cleaner, background app closer, temp cleaner, DNS flush.

Tweaks tagged FPS are the ones that help frame rate; "Apply FPS preset" turns on just those.

`NeroTweaks-FreeVersion.exe --selftest` applies and reverts every tweak and writes selftest.log.

## Selling Pro (free + one-time code locked to one PC)

Free works forever. Pro is unlocked by a code that only works on the PC it was made for.

1. Customer opens **Get Pro** in the app, clicks **Copy ID** and sends you the PC ID (`NERO-XXXX-XXXX-XXXX-XXXX`) with payment.
2. Put `tools/NeroCodeMaker.exe` and your secret `nero_private_key.bin` in one folder, double-click NeroCodeMaker.exe, paste their PC ID, and send back the code it prints (it is also copied to your clipboard). No Python needed. (`tools/keygen.py` does the same from the .pem if you prefer.)
3. Customer pastes it into the app and clicks **Activate Pro**.

The code is a signature of that PC's ID, so a friend with a different PC can't use it.
**Keep `nero_private_key.pem` secret and backed up** - anyone with it can make codes, and if you lose it you can't issue more for this build.
`tools/make_keys.py` makes a new key pair (then rebuild the exe with the new `pubkey.inc`).
Free vs Pro lists are in `license.inc` (`freeTweaks`, `itemLocked`).
