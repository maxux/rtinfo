var units  = ['b', 'Ko', 'Mo', 'Go', 'To', 'Po'];
var rates  = ['b/s', 'Ko/s', 'Mo/s', 'Go/s', 'To/s', 'Po/s'];
var batpic = ["→", "↓", "↑"];

var gradient = [[19, 55, 131], [12, 138, 51], [255, 75, 0], [255, 43, 48]];
var color;
var root;

var globaljson = null;
var globalview = null;

//
// gradient compution
//
function interp(i, segment, index) {
	return gradient[segment][index] - 
	       ((gradient[segment][index] - gradient[segment + 1][index]) * i);
}

//
// compute an array from gradient color
//
function grading(width) {
	var output = new Array();
	
	// cut width into gradients segments
	eachw = width / (gradient.length - 1);
	
	// foreach segments
	for(var z = 0; z < gradient.length - 1; z++) {
		// compute this segment gradient
		for(var i = 0; i < eachw; i++) {
			var temp = new Array();
			temp[0] = interp(i / eachw, z, 0).toFixed(0);
			temp[1] = interp(i / eachw, z, 1).toFixed(0);
			temp[2] = interp(i / eachw, z, 2).toFixed(0);
			
			output.push(temp.join());
		}
	}
	
	return output;
}

function percentstyle(percent, reverse) {
	if(reverse == undefined)
		reverse = false;
	
	if(reverse)
		percent = 100 - percent;
	
	return 'rgb(' + color[percent.toFixed(0)] + ');';
}

function percentvalue(value, total) {
	if(total == 0)
		return 0;
		
	return parseFloat(Math.floor((value / total) * 100));
}

function average(array) {
	var avg = 0;
	
	for(var i = 0; i < array.length; i++)
		avg += array[i];
	
	return avg / array.length;
}

//
// return a value prefixed by zero if < 10
//
function zerolead(value) {
	return (value < 10) ? '0' + value : value;
}

//
// convert a unix timestamp to readable european date/hours
//
function unixtimestr(timestamp) {
	var date = new Date(timestamp * 1000);
	
	hours = zerolead(date.getHours()) + ':' + 
	        zerolead(date.getMinutes()) + ':' + 
	        zerolead(date.getSeconds());

	date  = zerolead(date.getDate()) + '/' + 
	        zerolead(date.getMonth() + 1) + '/' + 
	        zerolead(date.getFullYear());
	
	return date  + ' ' + hours;
}

//
// compute a scaled size with adapted prefix
//
function size(value, total) {
	uindex = 1;
	
	pc = Math.floor((value / total) * 100);
	
	for(; value > 1024; value /= 1024)
		uindex++;
	
	text = value.toFixed(2) + ' ' + units[uindex] + (total ? ' (' + pc + ' %)' : '');

	return $('<span>', {style: percentstyle(pc)}).html(text);
}

function streamsize(value) {
	uindex = 0;
	
	for(; value > 1024; value /= 1024)
		uindex++;
	
	text = value.toFixed(2) + ' ' + units[uindex];
	
	pc = ((uindex / rates.length) * 100);
	
	return $('<span>', {style: percentstyle(pc)}).html(text);
}

function rate(value, scale) {
	/* // value should be in bytes, starting in ko
	uindex = 1;
	
	// compute color based on interface speed/capacity
	// if scale is unknown, setting it to 100 Mbps
	if(scale == 0)
		scale = 100;
	
	// starting in Ko/s
	if((value /= 1024) > 1) {
		// bytes -> Mbits	
		if((ratio = (((value * 40) / 1024) / scale) * 100) > 100)
			ratio = 100;
		
		var option = {style: percentstyle(ratio)};
		
	} else var option = {class: 'hide'};
	
	for(; value > 1024; value /= 1024)
		uindex++;

	// console.log(index);
	return $('<span>', option).html(value.toFixed(2) + ' ' + rates[uindex]);
	*/
	
	return 0;
}

function ifspeed(value) {
	if(!value)
		return '';
	
	if(value >= 1000)
		return (value / 1000) + ' Gbps';
		
	return value + ' Mbps';
}

function ipclass(ip) {
	if(ip.substring(0, 3) == '10.'    ||
	   ip.substring(0, 6) == '172.16' ||
	   ip.substring(0, 7) == '192.168')
		return 'private';
	
	return 'highlight';
}

//
// compute an uptime (days or hours supported)
//
function uptime(value) {
	if((days = value / 86400) > 1)
		return Math.floor(days) + ' days';
	
	return Math.floor(value / 3600) + ' hours';
}

//
// return a celcius degree if exists
//
function degree(value, limit) {
	if(!value)
		return '<small>[NA]</small>';
		
	return value + '°C';
}

//
// return formated percent format with different colors
// scaled with value. optional output text can be used
//
function percent(value, reverse) {
	span = $('<span>', {
		style: percentstyle(value, reverse)
		
	}).html(value + ' %');
	
	return span;
}

//
// parsing battery rtinfo object
//
function battery(battery) {
	if(battery.load == -1)
		return '<small>[AC]</small>';
	
	if(batpic[battery.status] != undefined)
		bat = batpic[battery.status] + ' ';
		
	return bat + percent(battery.load, true)[0].innerHTML;
}

//
// build a 'summary' table node line 
//
function summary_node(node, server) {
	var swap = 0;
	
	if(node.memory.swap_total > 0)
		swap = node.memory.swap_total - node.memory.swap_free;
	
	var usage = [
		node.cpu_usage[0],
		percentvalue(node.memory.ram_used, node.memory.ram_total),
		percentvalue(swap, node.memory.swap_total)
	];
	
	var usagepc = parseInt(average(usage));
	
	var bgcolor = 'background-color: ' + percentstyle(usagepc);
	var article = $('<article>', {style: bgcolor, onclick: 'showhost("' + node.hostname + '");'});
	
	article.append($('<h1>').html(node.hostname));
	article.append($('<h2>').html(usagepc + '%'));
	article.append($('<h3>').html(node.remoteip));
	
	/*
	
	if(node.offline)
		tr = $('<tr>', {class: 'offline'});
		
	else tr = $('<tr>', {class: 'online'});
	
	tr.append($('<td>').html(node.hostname));
	tr.append($('<td>').html(percent(node.cpu_usage[0])));
	tr.append($('<td>').html(size(node.memory.ram_used, node.memory.ram_total)));
	
	if(node.memory.swap_total > 0) {
		
		tr.append($('<td>').html(size(swap, node.memory.swap_total)));
		
	} else tr.append($('<td>').html('<small>[no swap]</small>'));
	
	for(var i = 0; i < node.loadavg.length; i++)
		node.loadavg[i] = parseFloat(node.loadavg[i]).toFixed(2);
		
	tr.append($('<td>').html(node.loadavg.join(', ')));
	tr.append($('<td>').html(uptime(node.uptime)));
	tr.append($('<td>').html(battery(node.battery)));
	
	tr.append($('<td>').html(
		degree(node.sensors.cpu.average) + ' / ' +
		degree(node.sensors.hdd.average)
	));
	
	tr.append($('<td>').html(unixtimestr(node.time)));
	tr.append($('<td>').html(node.remoteip));
	*/
	
	return article;
}

//
// build a network interface table interface line
//
function network_iface(hostname, interface) {
	tr = $('<tr>');
	tr.append($('<td>').html(hostname));
	tr.append($('<td>').html(interface.name));
	tr.append($('<td>').html(rate(interface.rx_rate, interface.speed)));
	tr.append($('<td>').html(streamsize(interface.rx_data)));
	tr.append($('<td>').html(rate(interface.tx_rate, interface.speed)));
	tr.append($('<td>').html(streamsize(interface.tx_data)));
	
	iclass = ipclass(interface.ip);
	tr.append($('<td>').html(interface.ip).addClass(iclass));
	
	tr.append($('<td>').html(ifspeed(interface.speed)));
	
	return tr;
}

//
// build a network interface table node line
//
function network_node(node, server) {
	var root = $('<div>');
	
	for(var i = 0; i < node.network.length; i++) {
		// skip disabled interface
		if(node.network[i].ip == '0.0.0.0')
			continue;
		
		// skip loopback
		if(node.network[i].ip == '127.0.0.1')
			continue;
			
		root.append(network_iface(node.hostname, node.network[i]));
	}
	
	// separator
	root.children('tr').last().addClass('last');
	
	if(node.offline)
		root.children('tr').addClass('offline');
		
	else root.children('tr').addClass('online');
	
	return root[0].innerHTML;
}

function summary(server) {
	for(var i = 0; i < server.rtinfo.length; i++)
		$('#root').append(summary_node(server.rtinfo[i], server));
}

function progressbar(value) {
	if(value < 30)
		var color = 'progress-bar-success';
		
	else if(value < 40)
		var color = 'progress-bar-info';
	
	else if(value < 55)
		var color = 'progress-bar-warning';
	
	else if(value < 80)
		var color = 'progress-bar-danger';
	
	
	return $('<div>', {'class': 'progress progress-danger'}).html(
		$('<div>', {
			'class': 'progress-bar ' + color,
			'role': 'progressbar',
			'aria-valuenow': value,
			'aria-valuemin': 0,
			'aria-valuemax': 100,
			'style': 'width: ' + value + '%'
		})
	);
}

function progressrow(title, value, info) {
	var root = $('<div>', {'class': 'row'});
	
	root.append($('<div>', {'class': 'col-md-2'}).html(title));
	root.append($('<div>', {'class': 'col-md-8'}).html(progressbar(value)));
	root.append($('<div>', {'class': 'col-md-1'}).html(value + '%'));
	root.append($('<div>', {'class': 'col-md-1'}).html((info ? info : '')));
	
	return root;
}

function row(title, value, info) {
	var root = $('<div>', {'class': 'row'});
	
	root.append($('<div>', {'class': 'col-md-2'}).html(title));
	root.append($('<div>', {'class': 'col-md-8'}).html(value));
	root.append($('<div>', {'class': 'col-md-2'}).html((info ? info : '')));
	
	return root;
}

function host(node) {
	var content = $('<div>', {'class': 'container'});
	
	content.append($('<h2>', {'class': 'text-center'}).html(node.hostname));
	content.append($('<div>', {'class': 'littlelink', 'onclick': 'showall();'}).html('(show all nodes)'));
	
	content.append(progressrow('CPU', node.cpu_usage[0]));
	
	var ram   = percentvalue(node.memory.ram_used, node.memory.ram_total);
	var swap  = node.memory.swap_total - node.memory.swap_free;
	var pswap = percentvalue(swap, node.memory.swap_total);
	
	content.append(progressrow('RAM', ram, size(node.memory.ram_used)));
	content.append(progressrow('SWAP', pswap, swap));
	
	content.append(row('System time', Date(node.time * 1000)));
	content.append(row('Load average', node.loadavg.join(', ')));
	content.append(row('Remote address', node.remoteip));
	content.append(row('Uptime', uptime(node.uptime)));
	
	content.append(row('CPU Temperature', node.sensors.cpu.average + '°C'));
	content.append(row('HDD Temperature', node.sensors.hdd.average + '°C'));
	
	$('#root').append(content);
}

function network(server) {
	$('#network tbody').empty();
	
	for(var i = 0; i < server.rtinfo.length; i++)
		$('#network').append(network_node(server.rtinfo[i], server));
}

//
// search for specific host on global json tree and display it
//
function singlehost(hostname) {
	// clear everything (one more time in some cases)
	root.html('');
	
	for(var i = 0; i < globaljson.rtinfo.length; i++)
		if(globaljson.rtinfo[i].hostname == hostname)
			host(globaljson.rtinfo[i]);
}

function showhost(hostname) {
	globalview = hostname;
	singlehost(hostname);
}

function showall() {
	globalview = null;
	parsing(globaljson);
}

//
// parsing new json tree and call required display process
//
function parsing(json) {
	globaljson = json;
	
	// clearing everyting
	$('body').attr('class', 'connected');
	$('#loader').hide();
	root.html('');
	
	// precomputing
	for(var i = 0; i < json.rtinfo.length; i++)
		json.rtinfo[i].offline = (json.rtinfo[i].lasttime < json.servertime - 30);
	
	if(globalview) {
		// single host
		singlehost(globalview);
		
	// summary all hosts
	} else summary(json);
	// network(json);
}

function update() {
	$.ajax({ url: '/rtinfo' }).done(parsing);
}
	
$(document).ready(function() {
	// compute an array of 100 colors code
	// this array will be used to access gradient color
	// for "value-level"
	color = grading(100);
	root  = $('section#root');
	
	setInterval("update()", 1000);
	update();
});
