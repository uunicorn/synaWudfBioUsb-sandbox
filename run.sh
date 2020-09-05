#!/bin/sh
set -e

DEVICE=$(lsusb | grep 'Fingerprint' | awk -F '[^0-9]+' '{ print "/dev/bus/usb/" $2 "/" $3 }')
PWD=$(pwd)
BASENAME=$(basename "$PWD")
XPFWEXT=$(ls ./*.xpfwext)

docker run --rm -it \
    --device $DEVICE \
    --env-file .env \
    -v $PWD:/$BASENAME \
    -v $PWD/usb.txt:/root/.wine/drive_c/usb.txt \
    -v $PWD/$XPFWEXT:/root/.wine/drive_c/system32/$XPFWEXT \
    wine:validity wine64 "/${BASENAME}/a.exe" "$@" | tee "$(date +%s)".log