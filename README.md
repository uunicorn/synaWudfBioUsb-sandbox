A standalone win64 program which attempts to imitate 
winbio framework by dynamically loading the `synaWudfBioUsb.dll` driver
and making ioctl requests to scan/enroll fingers.

To build you will need a 64bit version of mingw cross-compiler
and `winbio_ioctl.h/winbio_types.h` from wdk/visual studio/whatever.
Use `mk` script to rebuild the `a.exe`. Collecting all the required header files 
is a tedious process, so this project contains a pre-compiled version of `a.exe`.

To run:

* Clone and build [Wine with hacks](https://github.com/uunicorn/wine/tree/hacking/validiy-sensor) designed 
  specifically for this thing to work.
  Can be built in Docker, for example
```sh
$ git clone https://github.com/uunicorn/wine -b validiy-sensor && cd $_
$ docker build -t wine:validity .
```

* Download the Windows driver and extract contents with `innoextract`
* Copy `synaWudfBioUsb.dll` from the extracted windows driver to the current directory
* Copy `*.xpfwext` file from the extracted windows driver to the current directory
* Create a file `usb.txt` with your USB device id:
```
$ echo '138a:0097' > usb.txt`
```
* Determine your fingerprint USB bus and device path:
```
DEVICE=$(lsusb -d 138a: | awk -F '[^0-9]+' '{ print "/dev/bus/usb/" $2 "/" $3 }')
```
* Run `a.exe` inside the container with one of the following options:
```sh
$ ./run.sh [ nop | identify | enroll ] 
```
* It may "fail" the first couple of times. Whenever driver detects
  that device ownership has changed (host computer details have changed), 
  it needs to re-flash and re-calibrate the device to establish a new
  trust. After each step the USB device is reconnected. The current 
  winusb stub is not smart enough to detect device reconnects, so the 
  next driver's attempt to talk to the device will immediately fail.
  Just restart `a.exe` a couple of times before giving up.
