function connectToHomeAP() {
	fetch("/configureHomeNetConnect.json")
	.then((response) => response.json())
	.then((data) => organizeResults(data));
}

function organizeResults(jsonData) {
	console.log(JSON.stringify(jsonData))
	for (const i in jsonData.networks.ssids) {
		select = document.getElementById("ssidSelector");
		ssid = jsonData.networks.ssids[i];
		option = document.createElement('option');
		option.value = ssid.ssid;
		option.innerHTML = ssid.ssid + " (Signal Strength: " + ssid.rssi + "dB, Encryption: " + ssid.encryptionType + ")";
		if(i === 0) {
			option.defaultSelected = true;
		}
		select.appendChild(option);
	}
	select = document.getElementById("ssidSelector");
	if(select.options[select.selectedIndex] !== null && select.options[select.selectedIndex] !== undefined && select.options[select.selectedIndex].value) {
		// Associate a pop-in text entry boxe for user to enter their chosen AP's associated Password!
		input = document.createElement('input');
		input.type = "text";
		input.name = "ssidPassword";
		input.id = "ssidPassword";
		input.innerHTML = "Please enter Network Password here"
		input.value = "";
		placeHolder = document.getElementById('wifiPassField'); 
		placeHolder.appendChild(input);
	}
}

function makeHomeNetworkConfigurationRequest() {
	inputSelector = document.getElementById('ssidSelector');
	inputSsid = inputSelector.options[inputSelector.selectedIndex].value
	passSelector = document.getElementById('ssidPassword');
	inputPass = passSelector.value
	console.log('JS: inputSsid=' + inputSsid + ', inputPass=' + inputPass);
	fetch("/signInToHomeNet.json", {
		method: "POST",
		headers: {
			'Accept': 'text/json',
			'Content-Type': 'text/json'
		},
		//make sure to serialize your JSON body
		body: JSON.stringify({
			chosenSsid: inputSsid,
			enteredPassword: inputPass
		})
	})
	.then((response) => response.json())
	.then((data) => console.log(data));
}