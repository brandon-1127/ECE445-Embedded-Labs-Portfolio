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
var hour        = "";
var minute      = "";
var second      = "";
var mil_time    = "";

var clockState = { mode: 0, set: 0, temp: 0};

// TODO: update eid with your own.
var hostname        = "broker.emqx.io";
var port            = "8083";
var eid             = "bmg3364_rxw62"
var clientId        = "mqtt_ee445l_" + eid;

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
	const url = 'ws://broker.emqx.io:8083/mqtt';

	const options = {
		// Clean session
		clean: true,
		connectTimeout: 4000,
		// Authentication
		clientId: eid,
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
	var val = message.payloadString;
	var clockBox = document.getElementById("board-clock");
	var displays = ["hour_display", "minute_display", "second_display"]

	if (topic.endsWith("/hour")) hour = parseInt(val);
    else if (topic.endsWith("/min"))  minute  = val;
    else if (topic.endsWith("/sec"))  second  = val;
	else if (topic.endsWith("/format")) mil_time = parseInt(val);

	else if (topic.endsWith("/mode")) clockState.mode = parseInt(val);
    else if (topic.endsWith("/set"))  clockState.set  = parseInt(val);
    else if (topic.endsWith("/temp")) clockState.temp = parseInt(val);

	if (clockState.mode === 0) { // Normal Mode
        clockBox.style.backgroundColor = "#a2b2e1";
        displays.forEach(id => document.getElementById(id).style.color = "#07879e"); // Teal
    } 
    else if (clockState.mode === 1) { // Setting Mode
        clockBox.style.backgroundColor = "#a2b2e1";

		if (clockState.set === 1) {
			displays.forEach(id => document.getElementById(id).style.color = "red");
		} else {
			displays.forEach(id => document.getElementById(id).style.color = "yellow"); // Yellow
		}
	} 
    else if (clockState.mode === 2) { // Alarm Mode
        clockBox.style.backgroundColor = "red"; // Box turns red
        displays.forEach(id => document.getElementById(id).style.color = "black"); // Text turns black
    }

	if (document.getElementById("temp_display")) {
        document.getElementById("temp_display").innerHTML = (clockState.temp / 10).toFixed(1) + "°F";
    }

    document.getElementById("mode_display").innerHTML = (clockState.mode === 0) ? "Running" : "Editing";
    document.getElementById("setting_type").innerHTML = (clockState.mode === 0) ? "---" : (clockState.set === 0 ? "Clock" : "Alarm");

	// Logic for 12-hour display
	var displayHour = parseInt(clockState.hour);
	var format = "";
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

    // Creating object of the Date class
  
    // Variable to store AM / PM
    
    var period = "";
  
    // Assigning AM / PM according to current hour
    if (mil_time == 0) {
        if (currentHour < 12) {
            period = "AM";
            if (currentHour == 0) currentHour = 12;
        } else {
            period = "PM";
            if (currentHour != 12) currentHour = currentHour - 12;
        }
    }
  
    // Adding time elements to the div
	if (document.getElementById("hour_display")) {
        document.getElementById("hour_display").innerText = hour;
        document.getElementById("minute_display").innerText = minute;
        document.getElementById("second_display").innerText = second;
        document.getElementById("ampm_display").innerText = period;
    } else {
        document.getElementById("board-clock").innerText = hour + " : " + minute + " : " + second + " " + period;
    }

    // Set Timer to 1 sec (1000 ms)
    setTimeout(Board_Time, 1000);
}

function update(t) {
	if (t < 10) {
		return "0" + t;
	}
	else {
		return t;
	}
}

Board_Time();
