#!/bin/bash

# (C) Copyright 2023 Canonical Ltd.
# SPDX-License-Identifier: Apache-2.0

SNAP_FIRMWARES="$SNAP_COMMON/firmware.d"

# Setup the necessary folders for dfx-mgr to work
if [ ! -d $SNAP_FIRMWARES ]
then
  echo "$SNAP_FIRMWARES does not exists, creating..."
  mkdir -p $SNAP_FIRMWARES
fi

$SNAP/bin/dfx-mgrd $SNAP_DATA/config/daemon.conf $SNAP_FIRMWARES
