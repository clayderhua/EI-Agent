function addResult(message)
{
	result = document.getElementById("text-result");
	result.value = message + "\n" + result.value;
}

function clearResult()
{
	result = document.getElementById("text-result");
	result.value = "";
}

function clearDeviceList()
{
	var deviceList = document.getElementById("div-devices");
	
	while (deviceList.firstChild) {
		deviceList.removeChild(deviceList.firstChild);
	}
}

function selectAll()
{
	var isSelect = false;
	var devices = document.getElementById("div-devices").children;
	// get select all checkbox
	var rectangle = devices[0].children[0];
	var _checkbox = rectangle.children[0];
	if (_checkbox.checked) {
		isSelect = true;
	} else {
		isSelect = false;
	}
	
	for (i = 1; i < devices.length; i++) {
		rectangle = devices[i].children[0];
		_checkbox = rectangle.children[0];
		_checkbox.checked = isSelect;
	}
}

function recv_discover(error, stdout, stderr)
{
	var deviceCount = 0;

	console.log("stdout=" + stdout);
	console.log("stderr=" + stderr);
	if (error) {
		throw error;
	}
	
	// add select all checkbox
	var deviceList = document.getElementById("div-devices");
	var device = document.createElement('div');
	device.className = 'Rectangle-1';
	device.style = 'display: flex';
	var checkboxPara = document.createElement("p");
	var _checkbox = document.createElement("input");
	_checkbox.type = "checkbox";
	_checkbox.onclick = selectAll;
	var textPara = document.createElement("p");
	var textNode = document.createTextNode("Select All");
	
	deviceList.appendChild(device);
	device.appendChild(checkboxPara);
		checkboxPara.appendChild(_checkbox);
	device.appendChild(textPara);
		textPara.appendChild(textNode);

	//var teststr = "[LP] loop waiting, tv_sec=10, tv_usec=0..............................\nrecv_discoer_ack: addr=172.22.12.56, deviceID=00000001-0000-0000-0000-000bab8cbdd9\nrecv_discoer_ack: addr=172.22.12.57, deviceID=00000001-0000-0000-0000-000bab8cbdda\n[LP] loop waiting, tv_sec=10, tv_usec=0..............................\n[LP] done";
	//var lines = teststr.split('\n');
	var lines = stdout.split('\n');
	var styleIdx = 2;
	for(var i = 0;i < lines.length;i++)
	{
		// find ip
		var start = lines[i].indexOf("addr=");
		if (start == -1) continue; // not a addr info
		start += "addr=".length;
		
		var end = lines[i].indexOf(",", start);
		if (end == -1) continue; // not a valid addr

		var ip = lines[i].substring(start, end);
		
		// find device ID
		start = lines[i].indexOf("deviceID=", end);
		if (start == -1) continue; // not a addr info
		start += "deviceID=".length;
		
		var deviceID = lines[i].substring(start);
		// remove newline
		deviceID = deviceID.replace(/(?:\r\n|\r|\n)/g, '');
		
		// show the device
		console.log("[" + deviceID + "], " + "[" + ip + "]")
		
		// create checkbox dynamically
		var device = document.createElement('div');
		if (styleIdx == 1) {
			device.className = 'Rectangle-1';
			styleIdx = 2;
		} else {
			device.className = 'Rectangle-2';
			styleIdx = 1;
		}
		device.style = 'display: flex';
		
		var checkboxPara = document.createElement("p");
		var _checkbox = document.createElement("input");
		_checkbox.type = "checkbox";
		_checkbox.name = deviceID;
		_checkbox.value = ip;
		var deviceIdPara = document.createElement("p");
		var deviceIdText = document.createTextNode(deviceID);	
		var ipPara = document.createElement("p");
		var ipText = document.createTextNode(" (" + ip + ")");
		
		deviceList.appendChild(device);
		device.appendChild(checkboxPara);
			checkboxPara.appendChild(_checkbox);
		device.appendChild(deviceIdPara);
			deviceIdPara.appendChild(deviceIdText);
		device.appendChild(ipPara);
			ipPara.appendChild(ipText);
		
		deviceCount++;
	}
	
	// recover button
	var _button = document.getElementById("button-discover");
	_button.disabled = false;
	_button.className = "rectbutton";
	_button = document.getElementById("button-activeall");
	_button.disabled = false;
	_button.className = "rectbutton";
	if (deviceCount > 0) {
		_button = document.getElementById("button-active");
		_button.disabled = false;
		_button.className = "rectbutton";
	}
	
	var progress = document.getElementById("div-progress");
	progress.style.display = "none";
	
	addResult("discover found device: " + deviceCount);
}

function discover()
{
	const child = require('child_process').execFile;
	var executablePath;
	
	console.log("do discover...");
	addResult("do discover...");
	
	// gray out button
	var _button = document.getElementById("button-discover");
	_button.disabled = true;
	_button.className = "grayoutRectButton";
	_button = document.getElementById("button-activeall");
	_button.disabled = true;
	_button.className = "grayoutRectButton";
	_button = document.getElementById("button-active");
	_button.disabled = true;
	_button.className = "grayoutRectButton";
	
	// clear list
	clearDeviceList();
	
	// show loading icon
	var progress = document.getElementById("div-progress");
	progress.style.display = "block";
	
	// discovering
	console.log("start listen lptcpreceiver")
	if (process.platform == "win32") {
		 executablePath = "resources\\tools\\lptcpreceiver.exe";
	} else {
		executablePath = "resources/tools/lptcpreceiver";
	}
	var parameters = ["10"];
	child(executablePath, parameters, recv_discover);

	console.log("send mcast discover...please wait for " + parameters[0] + " sec.")
	addResult("send mcast discover...please wait for " + parameters[0] + " sec.");
	parameters = ["discover"];
	if (process.platform == "win32") {
		 executablePath = "resources\\tools\\lpmcastsender.exe";
	} else {
		executablePath = "resources/tools/lpmcastsender";
	}

	child(executablePath, parameters, (error, stdout, stderr) => {
		if (error) {
			throw error;
		}
		addResult("send discover...done");
		console.log(stdout);
		console.log(stderr);
	});
}

function activeall()
{
	console.log("do activeall!")
	addResult("activeall...");
	
	const child = require('child_process').execFile;
	var executablePath;
	const parameters = ["activeall"];
	
	console.log("start activeall")
	if (process.platform == "win32") {
		 executablePath = "resources\\tools\\lpactivesender.exe";
	} else {
		executablePath = "resources/tools/lpactivesender";
	}
	
	child(executablePath, parameters, (error, stdout, stderr) => {
		if (error) {
			throw error;
		}
		addResult("activeall...done");
		console.log(stdout);
		console.log(stderr);
	});
}

function active()
{
	console.log("do active!")
	var devices = document.getElementById("div-devices").children;
	var parameters = ["active"];
	const child = require('child_process').execFile;
	var executablePath;
	
	if (process.platform == "win32") {
		 executablePath = "resources\\tools\\lpactivesender.exe";
	} else {
		executablePath = "resources/tools/lpactivesender";
	}

	for (i = 1; i < devices.length; i++) {
		var rectangle = devices[i].children[0];
		var _checkbox = rectangle.children[0];
		
		if (_checkbox.checked) {
			parameters[0] = "active";
			parameters[1] = _checkbox.value; // ip
			parameters[2] = _checkbox.name; // device ID
			console.log("active " + _checkbox.name + " (" + _checkbox.value + ")");
			addResult("active " + _checkbox.name + " (" + _checkbox.value + ")...");
			child(executablePath, parameters, (error, stdout, stderr) => {
				if (error) {
					throw error;
				}
				addResult("active " + _checkbox.name + " (" + _checkbox.value + ")...done");
				console.log(stdout);
				console.log(stderr);
			});
		}
	}
}
