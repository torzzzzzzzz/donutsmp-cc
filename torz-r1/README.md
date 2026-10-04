# Torz R1

Electron app: pick or import a pattern, preview it, and (on Windows) have the mouse pull against it while the aim key is held.

## Run / test / build

```
npm install
npm start                 # run
npm test                  # pattern import + engine tests
npm run dist              # Windows installer (NSIS) + portable .exe  -> release/
npm run dist:installer    # installer only
npm run dist:portable     # portable only
```

Build the Windows `.exe` files **on Windows** (or Linux/macOS with Wine installed - electron-builder needs it to stamp the exe icon).

## Features

- **Aim key**: Right + Left Mouse Button, or Left only.
- **Smooth pull**: each shot's correction is applied in 1 px steps and finishes within the first half of the shot interval. Off = one jump per shot.
- **Fire rate (rpm)**: set it to your gun's real rate (per gun, remembered). If the pull falls behind your shots, raise it.
  Rough guide: emptying 30 rounds in 3 s is about 600 rpm.
- **Accuracy** and per-gun **Horizontal / Vertical strength** (0-200 %, remembered per gun).
- **Import** `.json`: `[[dx,dy],...]`, `[{"x":1,"y":-2},...]` or `{"name","rpm","pattern"}`.
  UTF-8 BOM, numeric strings and `{x,y}` / `{dx,dy}` shapes are accepted. Rejected with a reason: invalid JSON,
  empty pattern, more than 1000 shots, any shot that is not two numbers. Values beyond +/-200 px are limited,
  rpm outside 30-3000 falls back to 450, and the user is told.
- **Tray icon**: Show, Turn On/Off, Minimize to tray, Quit. Only one copy can run at a time.
- While running, the app asks Windows for a 1 ms timer and opts out of power throttling so it keeps pace when a game is in front.
- Mouse control is Windows only; elsewhere the app is a preview.

## Files

| File | Purpose |
|------|---------|
| `main.js` | window, tray, hotkey, IPC |
| `recoil.js` | engine (polls buttons, sends relative mouse moves via koffi) |
| `pattern.js` | import/validation shared by the UI and the tests |
| `torz-r1.html` | UI |
| `icon.png`, `build/icon.ico` | square app icons generated from `logo.png` |

## Ready-made patterns (`patterns/`)

Import with **Choose File**, then **Save pattern**.

| File | Notes |
|------|-------|
| `AK-screenshot.json` | Traced from a 30-bullet AK screenshot (bottom dot = bullet 1). 29 kicks between 30 bullets, 450 rpm, scaled 0.5 screenshot-px to mouse-px. Adjust with the strength sliders. |
| `AK-screenshot-reversed.json` | Same kicks in reverse order, for if the first file feels backwards. |

The rpm is an assumption (450, same as the built-in AK) - the screenshot doesn't contain it.
