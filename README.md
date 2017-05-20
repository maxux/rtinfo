# rtinfo
rtinfo is a lightweight collection of tools to centralize realtime system monitoring, using low bandwidth
small footprint and small amount of dependencies

## Contents
- `rtinfo-client`: remote udp client (using librtinfo to collects data)
- `rtinfod`: daemon receiving status from `rtinfo-client` and provide a json output over http
- `rtinfo-ncurses`: a ncurse based interface to shows json contents from `rtinfod`
- `rtinfo-webui`: some demo html files used to display the json.

For a better html version, please check [maxux/rtinfo-dashboard](https://github.com/maxux/rtinfo-dashboard)

# Dependencies

- `rtinfo-client`: depends on `librtinfo` (optional runtime dependency: `hddtemp`)
- `rtinfod`: depends on `librtinfo` and `jansson`
- `rtinfo-ncurses`: depends on `librtinfo` (compile-time), `jansson` and `libncurses`

Note:
- `librtinfo` can be found here: [maxux/librtinfo](https://github.com/maxux/librtinfo)

# How to use it

You need to compile each wanted sub-directory. Just type 'make' on it.

- Start a daemon: `./rtinfod/rtinfod`
- Connect a client: `./rtinfo-client/rtinfo-client --host 127.0.0.1`
- Connect it a remote: `./rtinfo-ncurses/rtinfo-ncurses --host 127.0.0.1`

**Warning:** there is no encryption for the udp packets.
If you are on a dangerous/untrusted network, please consider using rtinfo over securized tunnel or trusted network.

`rtinfod` sends data in json format, grab it with HTTP GET request to /json on the rtinfod remote socket.

Note: there is no real web server behind `rtinfod`, don't try to do anything else than `GET /json`

# Bugs
Feel free to send any pull requests to improve this project

# Contributors
Thanks to:
[@nado](https://github.com/nado),
[@bendardenne](https://github.com/bendardenne),
[@wget](https://github.com/wget),
[@zaibon](https://github.com/zaibon)
