#!/bin/sh

default_firmware()
{
  default_fw="$(snapctl get firmwares.default)"
  # TODO
  # if [ -z "$default_fw" ]; then
  #   default_fw="#executefwdetect script"
  #   set_default_firmware $default_fw
  # fi
  echo "$default_fw"
}

set_default_firmware()
{
  echo "$default_fw" > $SNAP_DATA/config/default_firmware
  snapctl set firmwares.default="$1"

  snapctl restart dfx-mgr.dfx-mgrd
}
