# rtinfo
rtinfo is a lightweight collection of tools to centralize realtime system monitoring, using low bandwidth
small footprint and small amount of dependencies

## Contents
- `rtinfo-client`: remote udp client (using librtinfo to collects data)
- `rtinfod`: daemon receiving status from `rtinfo-client` and provide a json output over http
- `rtinfo-ncurses`: a ncurse based interface to shows json contents from `rtinfod`
- `rtinfo-webui`: some demo html files used to display the json.

For a better html version, please check [maxux/rtinfo-dashboard](https://github.com/maxux/rtinfo-dashboard)

# Demonstration

Please take a look at this gallery to show rtinfo displays: [http://imgur.com/a/K544C](http://imgur.com/a/K544C)

# Dependencies

- `rtinfo-client`: depends on `librtinfo` (optional runtime dependency: `hddtemp`)
- `rtinfod`: depends on `librtinfo` and `jansson`
- `rtinfo-ncurses`: depends on `librtinfo` (compile-time), `jansson` and `libncurses`

Note:
- `librtinfo` can be found here: [maxux/librtinfo](https://github.com/maxux/librtinfo)

# How to use it

You need to compile each wanted sub-directory. Just type 'make' on them.

- Start a daemon: `./rtinfod/rtinfod`
- Connect a client: `./rtinfo-client/rtinfo-client --disk sd --host 127.0.0.1`
- Connect it a remote: `./rtinfo-ncurses/rtinfo-ncurses --host 127.0.0.1`

**Warning:** there is no encryption for the udp packets.
If you are on a dangerous/untrusted network, please consider using rtinfo over securized tunnel or trusted network.

`rtinfod` sends data in json format, grab it with HTTP GET request to /json on the rtinfod remote socket.

Note: there is no real web server behind `rtinfod`, don't try to do anything else than `GET /json`

# Performance and resources impact

## Network usage
On my laptop (2 ssd, 7 network interfaces), a full client output (summary, network and disk usage) is about
`689 bytes` (`128 bytes` for system information, `132 bytes` for disks and `429 bytes` for network usage).

Keep in mind that on theses `689 bytes`, you have: per disk read/write bytes and rates, per interface mac address,
ip address, bytes tranfered and current rate, etc.

Except if you have a lot of (+20 disks or +15 network interfaces) components, all UDP packet should be sent
in a single Ethernet frame. The payload size may be improved later by compression, probably.

About `700 bytes` per seconds makes an average of `~57 MiB` of data per day. For this project usecase, this is
is really peanuts.

## System resources impact

After running `rtinfo-client` for 7 days on my laptop, it used `13 minutes 40 seconds` of cpu times, which makes
an average cpu utilization of `0.13%` and `0.70 MiB` of RAM usage... for a realtime information system (1 second delay only).

# Tested Hardware

rtinfo-client was fully running on:
- Dell Inspiron and XPS Laptop
- Generic x86_64 motherboard
- SuperMicro motherboard
- Raspberry Pi (Zero, B+, 2B, 3B), ARM
- Linksys WRT54G, MIPS
- Packet.net nodes x86_64 and ARM
- OVH and Kimsufi servers
- BBox2 (Belgian ISP MIPS routerboard)
- Orange Pi
- Mellanox Motherboard with Quanta Enclosure

It should works out-of-box on any Linux system, without root privilege
(at least on vanilla kernel, with grsec for exemple, you need root privilege for some information)


# Bugs
Feel free to send any pull requests to improve this project

# Contributors
Thanks to:
[@nado](https://github.com/nado),
[@bendardenne](https://github.com/bendardenne),
[@wget](https://github.com/wget),
[@zaibon](https://github.com/zaibon)
