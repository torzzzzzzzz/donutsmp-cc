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

## v1.1.0

- **Presets** (replaces Restore Point page): save your current tweaks under a name with a Fortnite item as the icon, apply in one click. Free: 2 presets, Pro: unlimited. The Windows restore point button is still there.
- **Input-delay tweaks**: precise system timer, fast keyboard response, foreground app boost, plus an *Apply low-latency preset* button.
- **Nero Assistant** (replaces AI Chat): offline helper that reads your live stats and recommends tweaks. No internet, no API key.
- **In-app updater**: the app checks `version.txt` on GitHub at startup and shows an *Update* button when a newer version is published.
  To release an update: rebuild, bump `APP_VERSION` in `features.inc`, bump `version.txt`, commit and push both.
  `UPDATE_BASE` in `features.inc` points at the branch the files are served from - change it if you move to `main`.
- Fortnite item icons come from the public fortnite-api.com item list (needs internet only when picking an icon).
