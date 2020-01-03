# artnet2ftdi
Simple artnet-dmx gateway for use with a simple FTDI USB adapter.

Based on dmx-udp.c by Daniel Mack (https://gist.github.com/zonque/10b7b7183519bf7d3112881cb31b6133)

## Build
docker build . -t mrtncls/artnet2ftdi:20200103.1

## Run
docker run --network bridge -p 6454:6454/udp -t --privileged -v /dev/bus/usb:/dev/bus/usb mrtncls/artnet2ftdi:20200103.1
