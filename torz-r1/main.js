const { app, BrowserWindow, globalShortcut, ipcMain, Tray, Menu, nativeImage } = require('electron');
const path = require('path');
const fs = require('fs');
const { createNative, createEngine } = require('./recoil');

let win, tray;
const engine = createEngine(createNative());   // null native (non-Windows / load failure) = preview only

// ---- tiny settings file (userData/settings.json) ----
let settings = { minimizeToTray: false };
const settingsFile = () => path.join(app.getPath('userData'), 'settings.json');
function loadSettings() { try { settings = { ...settings, ...JSON.parse(fs.readFileSync(settingsFile(), 'utf8')) }; } catch (_) {} }
function saveSettings() { try { fs.writeFileSync(settingsFile(), JSON.stringify(settings)); } catch (_) {} }

function showWindow() {
  if (!win) return;
  if (win.isMinimized()) win.restore();
  win.show(); win.focus();
}

// ---- tray ----
function buildTrayMenu() {
  const on = engine.isOn();
  tray.setToolTip('Torz R1 — ' + (on ? 'On' : 'Off'));
  tray.setContextMenu(Menu.buildFromTemplate([
    { label: 'Show Torz R1', click: showWindow },
    { label: on ? 'Turn Off' : 'Turn On', click: () => win && win.webContents.send('toggle', true) },
    { label: 'Minimize to tray', type: 'checkbox', checked: settings.minimizeToTray,
      click: item => { settings.minimizeToTray = item.checked; saveSettings(); } },
    { type: 'separator' },
    { label: 'Quit', click: () => app.quit() }
  ]));
}
function makeTray() {
  const img = nativeImage.createFromPath(path.join(__dirname, 'icon.png')).resize({ width: 16, height: 16 });
  tray = new Tray(img);
  tray.on('click', showWindow);
  buildTrayMenu();
}

// ---- only one copy may run (two engines would double the mouse movement) ----
if (!app.requestSingleInstanceLock()) {
  app.quit();
} else {
  app.on('second-instance', showWindow);
  app.disableHardwareAcceleration();
  app.whenReady().then(() => {
    loadSettings();
    win = new BrowserWindow({
      width: 1000, height: 760, backgroundColor: '#05070a', autoHideMenuBar: true, title: 'Torz R1',
      icon: path.join(__dirname, 'icon.png'),
      webPreferences: { preload: path.join(__dirname, 'preload.js'), contextIsolation: true }
    });
    win.loadFile(path.join(__dirname, 'torz-r1.html')).catch(err => {
      win.loadURL('data:text/html,' + encodeURIComponent('<body style="background:#05070a;color:#4de1ff;font-family:sans-serif;padding:20px"><h2>Failed to load</h2><pre>' + String(err) + '</pre>'));
    });
    win.webContents.on('before-input-event', (_e, i) => { if (i.type === 'keyDown' && i.key === 'F12') win.webContents.toggleDevTools(); });
    win.on('minimize', () => { if (settings.minimizeToTray) win.hide(); });
    makeTray();
    engine.start();
  });

  // Renderer pushes the current pattern/settings here; the engine plays them back while the aim key is held.
  ipcMain.handle('set-state', (_e, state) => {
    const was = engine.isOn();
    engine.configure(state);
    if (tray && was !== engine.isOn()) buildTrayMenu();
    return engine.available;
  });
  // Global hotkey: toggles the On/Off switch.
  ipcMain.handle('set-hotkey', (_e, accel) => {
    globalShortcut.unregisterAll();
    if (!accel) return false;
    try { return globalShortcut.register(accel, () => win && win.webContents.send('toggle')); } catch (_) { return false; }
  });
  app.on('will-quit', () => { globalShortcut.unregisterAll(); engine.stop(); });
  app.on('window-all-closed', () => app.quit());
}
