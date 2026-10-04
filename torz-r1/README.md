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
- **Smooth pull**: each shot's correction is spread across the shot interval (1 px steps). Off = one jump per shot.
- **Accuracy** and per-gun **Horizontal / Vertical strength** (0-200 %, remembered per gun).
- **Import** `.json`: `[[dx,dy],...]`, `[{"x":1,"y":-2},...]` or `{"name","rpm","pattern"}`.
  UTF-8 BOM, numeric strings and `{x,y}` / `{dx,dy}` shapes are accepted. Rejected with a reason: invalid JSON,
  empty pattern, more than 1000 shots, any shot that is not two numbers. Values beyond +/-200 px are limited,
  rpm outside 30-3000 falls back to 450, and the user is told.
- **Tray icon**: Show, Turn On/Off, Minimize to tray, Quit. Only one copy can run at a time.
- Mouse control is Windows only; elsewhere the app is a preview.

## Files

| File | Purpose |
|------|---------|
| `main.js` | window, tray, hotkey, IPC |
| `recoil.js` | engine (polls buttons, sends relative mouse moves via koffi) |
| `pattern.js` | import/validation shared by the UI and the tests |
| `torz-r1.html` | UI |
| `icon.png`, `build/icon.ico` | square app icons generated from `logo.png` |
