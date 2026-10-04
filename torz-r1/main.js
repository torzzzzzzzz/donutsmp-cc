const { app, BrowserWindow, globalShortcut, ipcMain } = require('electron');
const path = require('path');
const { createNative, createEngine } = require('./recoil');
let win;
const engine = createEngine(createNative());   // null native (non-Windows / load failure) = simulator only
app.disableHardwareAcceleration();
app.whenReady().then(() => {
  win = new BrowserWindow({
    width: 1000, height: 760, backgroundColor: '#05070a', autoHideMenuBar: true, title: 'Torz R1',
    icon: path.join(__dirname, 'logo.png'),
    webPreferences: { preload: path.join(__dirname, 'preload.js'), contextIsolation: true }
  });
  win.loadFile(path.join(__dirname, 'torz-r1.html')).catch(err => {
    win.loadURL('data:text/html,' + encodeURIComponent('<body style="background:#05070a;color:#4de1ff;font-family:sans-serif;padding:20px"><h2>Failed to load</h2><pre>' + String(err) + '</pre>'));
  });
  win.webContents.on('before-input-event', (_e, i) => { if (i.type === 'keyDown' && i.key === 'F12') win.webContents.toggleDevTools(); });
  engine.start();
});
// Renderer pushes the current pattern/settings here; the engine plays them back while the aim key is held.
ipcMain.handle('set-state', (_e, state) => { engine.configure(state); return engine.available; });
// Global hotkey: toggles the On/Off switch.
ipcMain.handle('set-hotkey', (_e, accel) => {
  globalShortcut.unregisterAll();
  if (!accel) return false;
  try { return globalShortcut.register(accel, () => win && win.webContents.send('toggle')); } catch (_) { return false; }
});
app.on('will-quit', () => { globalShortcut.unregisterAll(); engine.stop(); });
app.on('window-all-closed', () => app.quit());
