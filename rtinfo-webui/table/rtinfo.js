var units  = ['b', 'KiB', 'MiB', 'GiB', 'TiB', 'PiB'];
var rates  = ['b/s', 'KiB/s', 'MiB/s', 'GiB/s', 'TiB/s', 'PiB/s'];
var batpic = ["→", "↓", "↑"];

var color;
var root;
var globaljson;

function percentstyle(percent) {
	return 'color: black;';
}

function percentvalue(value, total) {
	if(total == 0)
		return 0;
		
	return parseFloat(Math.floor((value / total) * 100));
}

function colorizesw(value, size) {
	if(size == 0)
		return {'class': ''};
	
	if(value < 8)
		return {'class': 'info'};
	
	return colorize(value);
}

function colorize(value) {
	if(value < 8)
		return {'class': ''};
		
	if(value < 50)
		return {'class': 'info'};
	
	if(value < 80)
		return {'class': 'warning'};
	
	return {'class': 'danger'};
}

function loadcolor(value, cpu) {
	if(value < 1.1)
		return {'class': ''};
		
	if(value < cpu / 4)
		return {'class': 'info'};
	
	if(value < cpu)
		return {'class': 'warning'};
	
	return {'class': 'danger'};
}

function autosize(value) {
	var temp = value / 1024;
	var unitidx = 2;
	
	if(temp > 4096) {
		temp /= 1024;
		unitidx = 3;
	}
	
	return temp.toFixed(2) + ' ' + units[unitidx];
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
function unixtime(timestamp) {
	var date = new Date(timestamp * 1000);
	
	var hours = zerolead(date.getHours()) + ':' + 
	            zerolead(date.getMinutes()) + ':' + 
	            zerolead(date.getSeconds());
	
	return hours;
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

function colorintf(value, maxspeed) {
	var value = value / 1024 / 1024; // let's compute everything in Mbps
	
	// compute color based on interface speed/capacity
	// if scale is unknown, setting it to 100 Mbps
	if(maxspeed == 0)
		scale = 100;
	
	// computing percentage of usage
	var pc = (value / maxspeed) * 100;
	
	if(value < 5)
		return {'class': ''};
		
	if(value < 40)
		return {'class': 'info'};
	
	if(value < 60)
		return {'class': 'warning'};
	
	return {'class': 'danger'};
}

function colordisk(value) {
	// using MB/s
	value = value / 1024 / 1024;
	
	if(value < 5)
		return {'class': ''};
		
	if(value < 75)
		return {'class': 'info'};
	
	if(value < 200)
		return {'class': 'warning'};
	
	return {'class': 'danger'};
}

function rate(value) {
	value = value / 1024 / 1024;
	uindex = 2;
	
	for(; value > 1024; value /= 1024)
		uindex++;
	
	return value.toFixed(2) + ' ' + rates[uindex];
}

function ifspeed(value) {
	if(!value)
		return '';
	
	if(value >= 1000)
		return (value / 1000) + ' Gbps';
		
	return value + ' Mbps';
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
function percent(value, extra) {
	return value + ' %' + ((extra) ? ' (' + extra + ')' : '');
}

//
// parsing battery rtinfo object
//
function battery(battery) {
	var bat = '';
	
	if(battery.load == -1)
		return '<small>[AC]</small>';
	
	if(batpic[battery.status] != undefined)
		bat = batpic[battery.status] + ' ';
		
	return bat + percent(battery.load);
}

//
// build a 'summary' table node line 
//
function summary_node(node, server) {
	var tr = $('<tr>', status);
	
	var status = (node.lasttime + 30 < server.servertime) ? 'danger' : 'success';
	tr.append($('<td>', {'class': status}).html(node.hostname));
	
	var swap = 0;
	if(node.memory.swap_total > 0)
		swap = node.memory.swap_total - node.memory.swap_free;
	
	for(var index in node.loadavg)
		node.loadavg[index] = parseFloat(node.loadavg[index]).toFixed(2);
	
	var nbcpu = node.cpu_usage.length - 1;
	var ram   = percentvalue(node.memory.ram_used, node.memory.ram_total);
	var swap  = node.memory.swap_total - node.memory.swap_free;
	var pswap = percentvalue(swap, node.memory.swap_total);
	
	tr.append($('<td>', colorize(node.cpu_usage[0]))
		.html(percent(node.cpu_usage[0]) + ' / ' + nbcpu));
	
	var size = autosize(node.memory.ram_used);
	tr.append($('<td>', colorize(ram)).html(percent(ram, size)));
	
	var size = autosize(swap);
	if(node.memory.swap_total > 0)
		tr.append($('<td>', colorizesw(pswap, swap)).html(percent(pswap, size)));
		
	else tr.append($('<td>').html('-'));
	
	tr.append($('<td>', loadcolor(node.loadavg[0], nbcpu)).html(node.loadavg[0]));
	tr.append($('<td>', loadcolor(node.loadavg[1], nbcpu)).html(node.loadavg[1]));
	tr.append($('<td>', loadcolor(node.loadavg[2], nbcpu)).html(node.loadavg[2]));
	tr.append($('<td>').html(node.remoteip));
	tr.append($('<td>').html(unixtime(node.time)));
	
	var up = uptime(node.uptime);
	tr.append($('<td>').html(up));
	
	var bat = battery(node.battery);
	tr.append($('<td>').html(bat));
	
	var temp = degree(node.sensors.cpu.average) + ' / ' + 
	           degree(node.sensors.hdd.average);

	tr.append($('<td>').html(temp));
	
	var speed = 0
	for(var idx in node.disks)
		speed += node.disks[idx].read_speed + node.disks[idx].write_speed;
	
	tr.append($('<td>', colordisk(speed)).html(rate(speed)));
	
	return tr;
}

function disk_node(node, disks, server) {
	var tr = $('<tr>', status);
	var local = {};
	
	for(var i in disks)
		local[i] = null;

	var status = (node.lasttime + 30 < server.servertime) ? 'danger' : 'success';
	tr.append($('<td>', {'class': status}).html(node.hostname));
	
	for(var idx in node.disks) {
		speed = node.disks[idx].read_speed + node.disks[idx].write_speed;
		local[node.disks[idx].name] = speed;
	}
	
	for(var i in local) {
		if(local[i] != null)
			tr.append($('<td>', colordisk(local[i])).html(rate(local[i])));

		else tr.append($('<td>', {'class': 'text-muted'}).html("-"));
	}
	
	return tr;
}

function diskhead(ndisks, disks) {
	var idx = Object.keys(disks).length;
	
	// if disk is unknown, adding it
	for(var d in ndisks)
		if(disks[ndisks[d].name] == undefined)
			disks[ndisks[d].name] = idx++;
	
	return disks;
}

//
// build a network interface table interface line
//
function network_intf(node, intf, server) {
	// skip disabled interface
	if(intf.ip == '0.0.0.0')
		return null;
	
	// skip loopback
	if(intf.name == 'lo')
		return null;
			
	tr = $('<tr>');
	
	var status = (node.lasttime + 30 < server.servertime) ? 'danger' : 'success';
	tr.append($('<td>', {'class': status}).html(node.hostname));
	
	tr.append($('<td>').html(intf.name));
	tr.append($('<td>', colorintf(intf.rx_rate, intf.speed)).html(rate(intf.rx_rate)));
	tr.append($('<td>').html(streamsize(intf.rx_data)));
	tr.append($('<td>', colorintf(intf.rx_rate, intf.speed)).html(rate(intf.tx_rate)));
	tr.append($('<td>').html(streamsize(intf.tx_data)));
	
	tr.append($('<td>').html(intf.ip));
	
	tr.append($('<td>').html(ifspeed(intf.speed)));
	
	return tr;
}

//
// build summary table
//
function summary(server, nodes) {
	$('#summary').empty();
	$('#summary').css('display', '');
	
	var thead = $('<thead>')
		.append($('<td>').html('Hostname'))
		.append($('<td>').html('CPU / Nb'))
		.append($('<td>').html('RAM'))
		.append($('<td>').html('SWAP'))
		.append($('<td>', {'colspan': 3}).html('Load Avg.'))
		.append($('<td>').html('Remote IP'))
		.append($('<td>').html('Time'))
		.append($('<td>').html('Uptime'))
		.append($('<td>').html('Bat.'))
		.append($('<td>').html('CPU / HDD'))
		.append($('<td>').html('DIsk IO (sum)'));
	
	$('#summary').append(thead);
	$('#summary').append($('<tbody>'));
	
	for(var n in nodes)
		$('#summary tbody').append(summary_node(nodes[n], server));
}

//
// build disks table
//
function disks(server, nodes) {
	//
	// building list of all disks
	//
	var disks = {};
	$('#disks').empty();
	$('#disks').css('display', '');
	$('#disks').append('<thead>');
	
	for(var n in nodes)
		disks = diskhead(nodes[n].disks, disks);
	
	// order them
	var temp = Object.keys(disks).sort();
	disks = {};
	
	$('#disks thead').empty();
	$('#disks thead').append($('<td>').html('Hostname'))
	
	for(var i in temp) {
		disks[temp[i]] = null;	
		$('#disks thead').append($('<td>').html(temp[i]))
	}
	
	//
	// display disk table
	//
	$('#disks').append('<tbody>');
	for(var n in nodes)
		$('#disks tbody').append(disk_node(nodes[n], disks, server));
}

//
// build network table
//
function network(server, nodes) {
	$('#network').empty();
	$('#network').css('display', '');
	
	var thead = $('<thead>')
		.append($('<td>').html('Hostname'))
		.append($('<td>').html('Interface'))
		.append($('<td>').html('Download Rate'))
		.append($('<td>').html('Download Size'))
		.append($('<td>').html('Upload Rate'))
		.append($('<td>').html('Upload Size'))
		.append($('<td>').html('Interface Address'))
		.append($('<td>').html('Link Speed'))
				
	$('#network').append(thead);
	$('#network').append($('<tbody>'));
	
	for(var n in nodes)
		for(var i in nodes[n].network)
			$('#network tbody').append(network_intf(nodes[n], nodes[n].network[i], server));
}

//
// parsing new json tree and call required display process
//
var __allowed = ["summary"];
var __remote = "be-g8-3";

function parsing(json) {
	globaljson = json;
	
	// clearing everyting
	$('body').attr('class', 'connected');
	
	//
	// ordering hostname
	//
	var hosts = [];
	var nodes = [];
	
	for(var x in json.rtinfo)
		hosts.push(json.rtinfo[x].hostname);
	
	hosts = hosts.sort();
	
	for(var n in hosts)
		for(var x in json.rtinfo)
			if(json.rtinfo[x].hostname == hosts[n])
				nodes.push(json.rtinfo[x]);
	
	//
	// iterate over differents part showable/hiddable
	//
	if($.inArray('summary', __allowed) != -1)
		summary(json, nodes);

	else $('table#summary').css('display', 'none');
	
	if($.inArray('network', __allowed) != -1)
		network(json, nodes);

	else $('table#network').css('display', 'none');
	
	if($.inArray('disks', __allowed) != -1)
		disks(json, nodes);
	
	else $('table#disks').css('display', 'none');
}

function update() {
	$.ajax({ url: '/rtinfo' }).done(parsing).error(function(x) { console.log(x) });
	
	// hide loading progressbar
	$('#progressbar').css('display', 'none');
}
	
$(document).ready(function() {
	setInterval("update()", 5000);
	
	var host = getParameterByName('endpoint');
	if(host)
		__remote = host;

	update();
});

//
// dynamic page
//
function getParameterByName(name) {
	var url = window.location.href;
	name = name.replace(/[\[\]]/g, "\\$&");
	
	var regex = new RegExp("[?&]" + name + "(=([^&#]*)|&|#|$)"),
	results = regex.exec(url);
	
	if(!results)
		return null;

	if(!results[2])
		return '';

	return decodeURIComponent(results[2].replace(/\+/g, " "));
}

function setView(target, root) {
	if($(root).hasClass('btn-default')) {
		$(root).addClass('btn-primary');
		$(root).removeClass('btn-default');
		
		__allowed.push(target);
		
	} else {
		$(root).removeClass('btn-primary');
		$(root).addClass('btn-default');
		
		__allowed.splice(__allowed.indexOf(target), 1);
	}
	
	parsing(globaljson);
}
