const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('path');
let si = null;
try { si = require('systeminformation'); } catch (e) {}

function createWindow() {
  const win = new BrowserWindow({
    width: 1280, height: 800, minWidth: 900, minHeight: 600,
    backgroundColor: '#000000', title: 'Nero Tweaks', autoHideMenuBar: true,
    webPreferences: { preload: path.join(__dirname, 'preload.js'), contextIsolation: true }
  });
  win.loadFile(path.join(__dirname, 'src', 'index.html'));
}

// Real system stats. Any value that can't be read comes back null and the UI shows "--".
ipcMain.handle('stats', async () => {
  if (!si) return null;
  const safe = p => p.catch(() => null);
  const [load, temp, speed, gfx, fs, mem, net, disks] = await Promise.all([
    safe(si.currentLoad()), safe(si.cpuTemperature()), safe(si.cpuCurrentSpeed()),
    safe(si.graphics()), safe(si.fsSize()), safe(si.mem()), safe(si.networkStats()), safe(si.diskLayout())
  ]);
  const g = gfx && gfx.controllers && gfx.controllers[0];
  const n = net && net[0];
  return {
    cpuTemp: temp && temp.main, cpuLoad: load && load.currentLoad, cpuSpeed: speed && speed.avg,
    gpuTemp: g && g.temperatureGpu, gpuLoad: g && g.utilizationGpu, gpuMem: g && g.memoryUsed != null ? g.memoryUsed / 1024 : null,
    diskHealth: disks && disks[0] && disks[0].smartStatus === 'Ok' ? 100 : null,
    diskUse: fs && fs[0] && fs[0].use,
    ram: mem && (mem.active / mem.total) * 100,
    netTotal: n && n.rx_sec != null ? (n.rx_sec + n.tx_sec) * 8 / 1e6 : null,
    netSent: n && n.tx_sec != null ? n.tx_sec * 8 / 1e6 : null,
    netRecv: n && n.rx_sec != null ? n.rx_sec * 8 / 1e6 : null
  };
});

app.whenReady().then(() => {
  createWindow();
  app.on('activate', () => BrowserWindow.getAllWindows().length || createWindow());
});
app.on('window-all-closed', () => { if (process.platform !== 'darwin') app.quit(); });
