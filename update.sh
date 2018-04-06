#!/bin/bash

git update
git submodule update --init --recursive
cd hardware/esp8266/tools
./get.py

