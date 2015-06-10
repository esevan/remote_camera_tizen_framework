#!/bin/bash

if [ $1 -eq 1 ]
then
sudo gbs build -A armv7l --include-all
else
sdb root on
sdb push ~/GBS-ROOT/local/repos/tizen2.2/armv7l/RPMS/remote_camera*.rpm /home/
sync
sdb shell "rpm -ivh --force --nodeps /home/remote_camera*.rpm"
sdb shell "sync"
sdb shell "reboot -f"
fi

