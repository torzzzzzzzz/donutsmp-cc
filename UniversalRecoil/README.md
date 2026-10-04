# Universal Recoil v2.0 (reconstructed from `UR_3_1.exe`)

`UR_3_1.exe` is a native Delphi 12 (VCL) program, so the original `.pas` source is not stored in it.
This folder is a reconstruction:

| File | How it was recovered |
|------|----------------------|
| `Unit2.dfm` | **Exact.** The form is embedded in the exe as an `RCDATA` resource; it was decoded from binary to text DFM. The form icon and the five preview images are included as hex. |
| `Unit2.pas` | **Reconstructed** by disassembling the form's event handlers (found via Delphi's RTTI method table). The logic, constants, INI keys and UI strings are taken from the machine code; local variable names, formatting and a few control-flow shapes are mine. The four trackbar handlers were jump tables (`case`) in the binary and are written here as equivalent arithmetic. |
| `UR_3_1.dpr` | **Reconstructed.** The exe embeds the VCL style `RUBYGRAPHITE`, so the project calls `TStyleManager.TrySetStyle('Ruby Graphite')`. |

Original version info: *Universal Recoil | 8code*, 1.0.0.0. The About label reads
"Universal Recoil v2.0 - 8code | Revolex | ThichQuangDuc".

## What it does

- **F1 / F2** turn the macro on / off (`Timer2` polls the keys every 1 ms).
- While on, `Timer1` (1 ms) checks the selected **Aim Key** (`Left Mouse Button`, or `Right + Left Mouse Button`).
  While it is held it calls `mouse_event(MOUSEEVENTF_MOVE, …)` twice per tick:
  first by (**Right**, **Down**), `Sleep(Delay)`, then by (**Left**, **Up**), `Sleep(Delay)`.
- Normal ranges are ±20 px per axis and 0–50 ms delay; **Advanced Mode** widens them to ±250 px and 0–5000 ms.
- **Save / Load** read and write an `.ini` with section `[Recoil]` and keys `Right`, `Down`, `Left`, `Up`, `Delay`.

## Differences from the original

- The original form class was named `TChrome_WingetWin_1` (a near-copy of Chrome's real
  `Chrome_WidgetWin_1` window class, so its window looks like a Chrome window to other programs).
  It is named `TMainForm` here.
- Not compiled: no Delphi toolchain was available when this was produced.
  Open `UR_3_1.dpr` in Delphi 12 and add the style under *Project > Options > Application > Appearance*
  if `Ruby Graphite` is not already installed.
