A standalone win64 program which attempts to imitate 
winbio framework by dynamically loading the `synaWudfBioUsb.dll` driver
and making ioctl requests to scan/enroll fingers.

To build you will need a 64bit version of mingw cross-compiler
and `winbio_ioctl.h/winbio_types.h` from wdk/visual studio/whatever.
Use `mk` script to rebuild the `a.exe`. Collecting all the required header files 
is a tedious process, so this project contains a pre-compiled version of `a.exe`.

To run:

* Clone and build [Wine with hacks](https://github.com/uunicorn/wine/tree/hacking/validiy-sensor) designed 
  specifically for this thing to work. No need to `make install`, Wine can run when executed from the build tree.
* Set environment variables similar to env.sh
* Download the Windows driver and extract contents with `innoextract`
* Copy `synaWudfBioUsb.dll` from the extracted windows driver to this location
* Copy `*.xpfwext` file from the extracted windows driver to `c:\windows\system32` inside youre new Wine prefix
* Within this prefix create a file `c:\usb.txt` with your USB device id. E.g: `echo '138a:0097' > ~/.wine-testbed/drive_c/usb.txt`
* Run `<wine build tree>/wine64 a.exe 2>&1 | tee logfile.txt`
* It may "fail" the first couple of times. Whenever driver detects
  that device ownership has changed (host computer details have changed), 
  it needs to re-flash and re-calibrate the device to establish a new
  trust. After each step the USB device is reconnected. The current 
  winusb stub is not smart enough to detect device reconnects, so the 
  next driver's attempt to talk to the device will immediately fail.
  Just restart `a.exe` a couple of times before giving up.
