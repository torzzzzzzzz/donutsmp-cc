const { contextBridge, ipcRenderer } = require('electron');
contextBridge.exposeInMainWorld('nero', { stats: () => ipcRenderer.invoke('stats') });
