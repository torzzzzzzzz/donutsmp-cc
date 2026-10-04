const { contextBridge, ipcRenderer } = require('electron');
contextBridge.exposeInMainWorld('torz', {
  setHotkey: a => ipcRenderer.invoke('set-hotkey', a),
  setState: s => ipcRenderer.invoke('set-state', s),
  onToggle: cb => ipcRenderer.on('toggle', cb)
});
