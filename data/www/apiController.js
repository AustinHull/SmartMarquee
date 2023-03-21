function connectToHomeAP() {
	fetch("/configureHomeNetConnect.json")
	.then((response) => response.json())
	.then((data) => organizeResults(data)
		/*for (const i in data.networks.ssids) {
			select = document.getElementById("ssidSelector");
			ssid = data.networks.ssids[i];
			option = document.createElement('option');
			option.value = ssid.ssid;
			option.innerHTML = ssid.ssid + " (Signal Strength: " + ssid.rssi + "Encryption: " + ssid.encryptionType + ")";
			if(i === 0) {
				option.defaultSelected = true;
			}
			select.appendChild(option);
		}*/);
}

function organizeResults(jsonData) {
	for (const i in jsonData.networks.ssids) {
		select = document.getElementById("ssidSelector");
		ssid = jsonData.networks.ssids[i];
		option = document.createElement('option');
		option.value = ssid.ssid;
		option.innerHTML = ssid.ssid + " (Signal Strength: " + ssid.rssi + "Encryption: " + ssid.encryptionType + ")";
		if(i === 0) {
			option.defaultSelected = true;
		}
		select.appendChild(option);
	}
}