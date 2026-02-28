/*
   Eclipse Paho MQTT-JS Utility
   by Elliot Williams for Hackaday article

   Hacked up by:  Mark McDermott for EE445L Lab 4E
   On:            5/29/23   
*/

// ---------------------------------------
// Global variables
//

var client      = null;
var hour        = "00";
var minute      = "00";
var second      = "00";
var mil_time    = 0;

var clockState = { hour: "00", min: "00", sec: "00", mode: 0, set: 0, temp: 0, format: 0 };

// TODO: update eid with your own.
var hostname        = "broker.emqx.io";
var port            = "8083";
var eid             = "bmg3364_rxw62"
var clientId        = "mqtt_ee445l_" + eid + "_" + Math.random().toString(16).slice(2, 10);

// Subscribe (from board_2_webpage)

/*
var test 	= 	eid + "/test";
var hour_bd =   eid + "/b2w/hour"; 
var min_bd  =   eid + "/b2w/min";
var sec_bd  =   eid + "/b2w/sec"; 
var mode_bd =   eid + "/b2w/mode"   
*/

var b2w = eid + "/b2w";
// Publish (from webpage_2_board) 
 
var w2b   =  eid + "/w2b";

// -----------------------------------------------------------------------
// This is called after the webpage is completely loaded
// It is the main entry point into the JS code

function connect() {
	// Set up the client
	// TODO: We use a default here for emqx, but if you're using ECE445L broker,
	// feel free to replace with the hostname + port specified earlier. 
	const url = "ws://" + hostname + ":" + port + "/mqtt";

	const options = {
		// Clean session
		clean: true,
		connectTimeout: 4000,
		// Authentication
		clientId: clientId,
		username: null,
		password: null,
	};
	client  = mqtt.connect(url, options);
	client.on('connect', function () {
		onConnect();
	});

	// Receive messages
	client.on('message', function (topic, payload, packet) {
	  	onMessageArrived(topic, payload);
	});
}

function onConnect() {
	console.log("Client Connected.");
    
	client.subscribe(eid + "/b2w/#");
}

function onMessageArrived(topic, message) {
	var val = (message && message.toString) ? message.toString().trim() : String(message).trim();
	var clockBox = document.getElementById("board-clock");
	var displays = ["hour_display", "minute_display", "second_display"];

	if (topic.endsWith("/hour")) {
		clockState.hour = update(parseInt(val, 10) || 0);
		hour = clockState.hour;
	} else if (topic.endsWith("/min")) {
		clockState.min = update(parseInt(val, 10) || 0);
		minute = clockState.min;
	} else if (topic.endsWith("/sec")) {
		clockState.sec = update(parseInt(val, 10) || 0);
		second = clockState.sec;
	} else if (topic.endsWith("/format")) {
		clockState.format = parseInt(val, 10) || 0;
		mil_time = clockState.format;
	}

	else if (topic.endsWith("/mode")) clockState.mode = parseInt(val, 10) || 0;
    else if (topic.endsWith("/set"))  clockState.set  = parseInt(val, 10) || 0;
    else if (topic.endsWith("/temp")) clockState.temp = parseInt(val, 10) || 0;

	if (clockBox && clockState.mode === 0) { // Normal Mode
        clockBox.style.backgroundColor = "#a2b2e1";
        displays.forEach(id => {
			var el = document.getElementById(id);
			if (el) el.style.color = "#07879e";
		}); // Teal
    } 
    else if (clockBox && clockState.mode === 1) { // Setting Mode
        clockBox.style.backgroundColor = "#a2b2e1";

		if (clockState.set === 1) {
			displays.forEach(id => {
				var el = document.getElementById(id);
				if (el) el.style.color = "red";
			});
		} else {
			displays.forEach(id => {
				var el = document.getElementById(id);
				if (el) el.style.color = "yellow";
			}); // Yellow
		}
	} 
    else if (clockBox && clockState.mode === 2) { // Alarm Mode
        clockBox.style.backgroundColor = "red"; // Box turns red
        displays.forEach(id => {
			var el = document.getElementById(id);
			if (el) el.style.color = "black";
		}); // Text turns black
    }

	if (document.getElementById("temp_display")) {
        document.getElementById("temp_display").innerHTML = (clockState.temp / 10).toFixed(1) + "°F";
    }
	updateThermometer(clockState.temp);

	if (document.getElementById("mode_display")) {
		document.getElementById("mode_display").innerText = (clockState.mode === 0) ? "Normal" : (clockState.mode === 1) ? "Set" : "Alarm";
	}
	if (document.getElementById("setting_type")) {
		document.getElementById("setting_type").innerText = (clockState.set === 0) ? "Clock" : "Alarm";
	}

	Board_Time();
}


// -----------------------------------------------------------------------
// Provides the button logic that toggles the mode
// Triggered by pressing the HTML button "12/24"
//
function toggleTime() {
	var payload = "toggleTime";
	console.log("Publishing ", payload, " to ", w2b);
	client.publish(w2b, payload);
}

// //////////////////////////////////////////////////////////////////////////
//
//  ADD MORE FUNCTIONS HERE
//
// //////////////////////////////////////////////////////////////////////////

function toggleMode(){
	var payload = "toggleMode";
	console.log("Publishing ", payload, " to ", w2b);
	client.publish(w2b, payload);
}

function toggleSetting(){
	var payload = "toggleSetting";
	console.log("Publishing ", payload, " to ", w2b);
	client.publish(w2b, payload);
}

function reset(){
	var payload = "reset";
	console.log("Publishing ", payload, " to ", w2b);
	client.publish(w2b, payload);
}

function decMin(){
	var payload = "decMin";
	console.log("Publishing ", payload, " to ", w2b);
	client.publish(w2b, payload);
}

function incMin(){
	var payload = "incMin";
	console.log("Publishing ", payload, " to ", w2b);
	client.publish(w2b, payload);
}

function decHour(){
	var payload = "decHour";
	console.log("Publishing ", payload, " to ", w2b);
	client.publish(w2b, payload);
}

function incHour(){
	var payload = "incHour";
	console.log("Publishing ", payload, " to ", w2b);
	client.publish(w2b, payload);
}
// -----------------------------------------------------------------------
// This function appends AM or PM to the time when not in 24 hour mode
//
function Board_Time() {
	var displayHourNum = parseInt(clockState.hour, 10) || 0;
	var period = "";

	if (clockState.format === 0) {
		period = (displayHourNum < 12) ? "AM" : "PM";
		if (displayHourNum === 0) displayHourNum = 12;
		else if (displayHourNum > 12) displayHourNum -= 12;
	}

	var displayHour = update(displayHourNum);

	if (document.getElementById("hour_display")) {
		document.getElementById("hour_display").innerText = displayHour;
		document.getElementById("minute_display").innerText = clockState.min;
		document.getElementById("second_display").innerText = clockState.sec;
		document.getElementById("ampm_display").innerText = period;
	} else if (document.getElementById("board-clock")) {
		document.getElementById("board-clock").innerText = displayHour + " : " + clockState.min + " : " + clockState.sec + " " + period;
	}
}

function Internet_Time() {
	var date = new Date();
	var h = date.getHours();
	var m = date.getMinutes();
	var s = date.getSeconds();
	var period = "";

	if (clockState.format === 0) {
		period = (h < 12) ? "AM" : "PM";
		if (h === 0) h = 12;
		else if (h > 12) h -= 12;
	}

	var hourText = update(h);
	var minText = update(m);
	var secText = update(s);

	if (document.getElementById("internet_hour_display")) {
		document.getElementById("internet_hour_display").innerText = hourText;
		document.getElementById("internet_minute_display").innerText = minText;
		document.getElementById("internet_second_display").innerText = secText;
		document.getElementById("internet_ampm_display").innerText = period;
	} else if (document.getElementById("digital-clock")) {
		document.getElementById("digital-clock").innerText = hourText + " : " + minText + " : " + secText + " " + period;
	}
}

function update(t) {
	if (t < 10) {
		return "0" + t;
	}
	else {
		return t;
	}
}

function updateThermometer(tempTenthsF) {
	var fill = document.getElementById("thermo_fill");
	var label = document.getElementById("thermo_text");
	if (!fill) return;

	var tempF = (tempTenthsF || 0) / 10.0;
	var minF = 50;
	var maxF = 90;
	var percent = ((tempF - minF) / (maxF - minF)) * 100;
	if (percent < 0) percent = 0;
	if (percent > 100) percent = 100;

	fill.style.height = percent.toFixed(1) + "%";

	if (percent < 33) {
		fill.style.background = "linear-gradient(to top, #2b6de9, #47c9ff)";
	} else if (percent < 66) {
		fill.style.background = "linear-gradient(to top, #21a643, #8de05c)";
	} else {
		fill.style.background = "linear-gradient(to top, #e26a1b, #e33636)";
	}

	if (label) {
		label.innerText = tempF.toFixed(1) + "°F";
	}
}

Board_Time();
Internet_Time();
setInterval(Internet_Time, 1000);
