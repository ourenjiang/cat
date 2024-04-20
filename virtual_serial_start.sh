#!/bin/sh

socat pty,raw,echo=0,link=/dev/ttyS11,b38400 pty,raw,echo=0,link=/dev/ttyS12,b38400
