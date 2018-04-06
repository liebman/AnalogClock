#!/bin/bash

git submodule update --init --recursive
cd hardware/esp8266com/esp8266/tools
./get.py

