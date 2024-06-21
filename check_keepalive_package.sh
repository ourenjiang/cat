#!/bin/sh

sudo tcpdump -i ens33 tcp and \(src host 192.168.49.138 and src port 58046\) and \(dst host 192.168.10.251 and dst port 19000\)