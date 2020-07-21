const {
    app,
    BrowserWindow,
    dialog,
    globalShortcut,
    ipcMain,
    nativeImage
} = require('electron')

const path = require('path')
const url = require('url')

app.on('ready', () => {
	console.log("Register Debug")
    ret = globalShortcut.register('CommandOrControl+F12', () => {
        console.log("Enable Debug")
        EnableDebug()
    })
	if (!ret) {
		console.log("Register debug fail!")
	}
 
    createWindow()
})

app.on('window-all-closed', () => {
    // darwin = MacOS
    if (process.platform !== 'darwin') {
        app.quit()
    }
})

app.on('activate', () => {
    if (win === null) {
        createWindow()
    }
})

function createWindow() {
    // Create the browser window.
    win = new BrowserWindow({
        width: 650,
        height: 600,
        maximizable: false,
		webPreferences: { // for app.js can call require()
            nodeIntegration: true
        }
    })
	win.setMenu(null)
	
    win.loadURL(url.format({
        pathname: path.join(__dirname, 'index.html'),
        protocol: 'file:',
        slashes: true
    }))

    // Open DevTools.
	/*
    win.webContents.openDevTools()
    win.setMaximizable(true)
    win.setResizable(true)
	*/
	
    // When Window Close.
    win.on('closed', () => {
        win = null
    })
}

function EnableDebug() {
    // Open DevTools.
    win.webContents.openDevTools()
    win.setMaximizable(true)
    win.setResizable(true)
}