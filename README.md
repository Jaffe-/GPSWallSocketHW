# GPSWallSocketHW
Hardware part of the GPS Wall Socket project

## Setup on Windows

* Install the [Arduino software](https://www.arduino.cc/en/Main/Software). We need it for the GCC compiler and the AVRDUDE flash programming software. The Arduino IDE also has a serial monitor that can be useful.
* Install the [MinGW Installation Manager](https://sourceforge.net/projects/mingw/files/Installer/mingw-get-setup.exe/download). MinGW is a set of GNU utilities that have been ported to Windows. Run it, and from the left menu select MSYS and MSYS Base System. Install the following (select the class 'bin'):
  * bash
  * coreutils
  * make
* Add the necessary paths to the system PATH variable:
  * \<Arduino Installation Directory>\hardware\tools\avr\bin
  * \<Arduino Installation Directory>\hardware\tools\avr\etc
  * \<MinGW Installation Directory>\msys\1.0\bin
  
  At the top left, click Installation -> Apply Changes to start installation.

Unless anything has been forgotten, this should be enough to be able to build the project. To test that the tools have been installed and set up correctly, try opening CMD and running `make`, `ls`, `avrdude` and `avr-gcc`.

## Compiling and flashing

To compile, navigate to the directory and run `make`.

To flash, run `make PORT=<COM port> flash`, with the name of the Arduino's COM port (or the device file on Linux).

To clean up temporary build files, run `make clean`.
