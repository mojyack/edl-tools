# edl-tools
Utilities for qualcomm emergency download mode  
Tested with OnePlus 6  
edl-client: sahara & firehose protocol interpreter  
edl-buse: network block device based on firehose read/program command

# Why
There is great bkerler's edl tool(https://github.com/bkerler/edl).  
But this didn't work in my environment for unknown reasons, so I decided to recreate it for my own study.

# Build
```
meson setup build --buildtype release
ninja -C build
```
# Typical usage
Put your edl loader in "./loader.bin"  
Assume the edl mode device is /dev/ttyUSB0
## Mount internal storage
```
// open interpreter
% build/client /dev/ttyUSB0
// upload programmer
EDL% upload
// configure programmer
EDL% fhconf
// exit interpreter
EDL% exit
// ensure nbd module loaded
% modprobe nbd
// start nbd server for lun 0
% build/buse /dev/ttyUSB0 0
// now /dev/nbd0(p*) should appeared
// you can use any tools like gdisk, mkfs, mount...

// in another terminal
// stop nbd server
% nbd-client -d /dev/nbd0
// open interpreter and reboot device
% build/client /dev/ttyUSB0
EDL% fhreset
```

# Credits
Written based on this:  
https://github.com/bkerler/edl  
edl-buse uses c++ rewritten acozzette's BUSE:  
https://github.com/acozzette/BUSE
