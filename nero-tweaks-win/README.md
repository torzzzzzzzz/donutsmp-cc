# Nero Tweaks (native Windows app, C)

`NeroTweaks.exe` is a single file - just double-click it. No install needed.

Rebuild from source on Linux: `sudo apt install mingw-w64 && ./build.sh`

- Real live stats: CPU usage/speed, RAM, disk, network. GPU via `nvidia-smi` (NVIDIA only).
- Settings (name, toggles) saved in `%APPDATA%\NeroTweaks\config.ini`.
- Restore Point uses Windows `Checkpoint-Computer` (needs Run as administrator).
- Tweak toggles are saved but do not yet change Windows settings.
