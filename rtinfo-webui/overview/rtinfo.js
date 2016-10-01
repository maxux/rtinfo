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
function render(stats, target) {
	var tr = $('<tr>');
	tr.append($('<td>').html('Nodes'))
	tr.append($('<td>').html(stats['nodes']))
	tr.append($('<td>').html("Load Avg."))
	
	var ldavg = [
		stats['load'][0].toFixed(1),
		stats['load'][1].toFixed(1),
		stats['load'][2].toFixed(1)
	];

	tr.append($('<td>', loadcolor(stats['load'][0], stats['cpu'])).html(ldavg.join(", ")))
	$(target).append(tr)
	
	var tr = $('<tr>');
	tr.append($('<td>').html('CPU Cores'))
	tr.append($('<td>').html(stats['cpu']))
	tr.append($('<td>').html('Load'))
	tr.append($('<td>').html(percent(stats['cpu_usage'])))
	$(target).append(tr)
	
	var tr = $('<tr>');
	tr.append($('<td>').html('RAM'))
	tr.append($('<td>').html(size(stats['ram_total'])))
	tr.append($('<td>').html('Load'))
	tr.append($('<td>', colorize(stats['ram_load'])).html(percent(stats['ram_load'], autosize(stats['ram_used']))))
	$(target).append(tr)
	
	var tr = $('<tr>');
	tr.append($('<td>').html('SWAP'))
	tr.append($('<td>').html(size(stats['swap_total'])))
	tr.append($('<td>').html('Load'))
	tr.append($('<td>', colorize(stats['swap_load'])).html(percent(stats['swap_load'], autosize(stats['swap_used']))))
	$(target).append(tr)
	
	var tr = $('<tr>');
	tr.append($('<td>').html('Disks'))
	tr.append($('<td>').html(stats['disks']))
	tr.append($('<td>').html('IOPS'))
	tr.append($('<td>').html(stats['disks_iops']))
	$(target).append(tr)
	
	var tr = $('<tr>');
	tr.append($('<td>').html('Disks Read'))
	tr.append($('<td>', colordisk(stats['disks_read'])).html(rate(stats['disks_read'])))
	tr.append($('<td>').html('Disks write'))
	tr.append($('<td>', colordisk(stats['disks_write'])).html(rate(stats['disks_write'])))
	$(target).append(tr)
}

//
// parsing new json tree and call required display process
//
var __allowed = ["summary"];
var __remote = "be-g8-4";

function stats(rtinfo, prefix) {
    var __stats = {
        'nodes': 0,
        'ram_total': 0,
        'load': [0, 0, 0],
        'ram_used': 0,
        'swap_total': 0,
        'swap_used': 0,
        'cpu': 0,
        'cpu_usage': 0,
        'disks': 0,
        'disks_read': 0,
        'disks_write': 0,
        'disks_iops': 0
    }

    for(var h in rtinfo['rtinfo']) {
		var host = rtinfo['rtinfo'][h]

		// ignore prefix if prefix is set
        if(prefix != "") {
            if(!host['hostname'].startsWith(prefix))
                continue
        }

        __stats['nodes'] += 1
        __stats['cpu'] += host['cpu_usage'].length - 1
        __stats['cpu_usage'] += host['cpu_usage'][0]
        
        __stats['load'][0] += host['loadavg'][0]
        __stats['load'][1] += host['loadavg'][1]
        __stats['load'][2] += host['loadavg'][2]

        __stats['ram_total'] += host['memory']['ram_total']
        __stats['ram_used'] += host['memory']['ram_used']
        __stats['swap_total'] += host['memory']['swap_total']
        __stats['swap_used'] += host['memory']['swap_total'] - host['memory']['swap_free']

        __stats['disks'] += host['disks'].length

        for(var d in host['disks']) {
			var disk = host['disks'][d]

            __stats['disks_read'] += disk['read_speed']
            __stats['disks_write'] += disk['write_speed']
            __stats['disks_iops'] += disk['iops']
        }
    }

	var temp = (__stats['cpu_usage'] / __stats['nodes']) / 100
    __stats['cpu_usage'] = percentvalue(temp, 1)
    __stats['ram_total_gb'] = __stats['ram_total'] / (1024.0 * 1024)
    __stats['ram_load'] = percentvalue(__stats['ram_used'], __stats['ram_total'])
    __stats['swap_total_gb'] = __stats['swap_total'] / (1024.0 * 1024)
    __stats['swap_load'] = percentvalue(__stats['swap_used'], __stats['swap_total'])
    __stats['disks_read_mbps'] = __stats['disks_read'] / (1024.0 * 1024)
    __stats['disks_write_mbps'] = __stats['disks_write'] / (1024.0 * 1024)
    
    return __stats
}

function inTitle(target, name) {
	var tr = $('<tr>')
	
	var attr = {
		'class': 'active table-title',
		'colspan': 4,
	}
	
	tr.append($('<td>', attr).html(name));

	$(target).append(tr)
}

function parsing(json) {
	globaljson = json;
	
	// clearing everyting
	$('body').attr('class', 'connected');
	
	//
	// computing stats
	//
	full = stats(json, "")
	cpu = stats(json, "cpu")
	stor = stats(json, "stor")
	
	var target = '#global'

	$(target).empty();
	$(target).css('display', '');
	$(target).append($('<tbody>'));
	
	target = '#global tbody'
	
	inTitle(target, "Full Environment")
	render(full, target);

	//
	// for exemple, make summary for hostname begening with "cpu" and "stor"
	//
	/*	
	inTitle(target, "CPU Nodes")
	render(cpu, target);
	
	inTitle(target, "Storage Nodes")
	render(stor, target);
	*/
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
	parsing(globaljson);
}
