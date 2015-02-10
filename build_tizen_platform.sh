#!/bin/bash

sudo gbs build -A armv7l --include-all --threads 1
if test $(sdb devices | wc -l) -lt 2
then
	echo "No sdb devices found"
else
sdb root on
	sdb push ~/GBS-ROOT/local/repos/tizen2.2/armv7l/RPMS/app-prefetcher*.rpm /home/
	sdb shell "rpm -Uvh --force --nodeps /home/app-prefetcher*.rpm"
fi

