# GPSWallSocketHW
Hardware part of the GPS Wall Socket project

`node` contains the code for the Arduino nodes.
`hub` contains the radio interface for the Raspberry Pi.

## Raspberry pi

The nRF905 linux driver had to be altered so that it buffers the input and allows for polling the device file. The patched driver is found here: https://github.com/Jaffe-/nRF905. The build/install instructions are still the same.
