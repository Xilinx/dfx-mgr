#!/bin/bash
set -x
cmake ../ \
	-DCMAKE_INCLUDE_PATH="$(pwd)/../../include" \

