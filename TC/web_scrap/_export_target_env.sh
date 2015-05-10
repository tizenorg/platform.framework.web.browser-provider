#!/bin/sh
. ./config

export TET_INSTALL_PATH=$TET_INSTALL_TARGET_PATH # path to path
#export TET_TARGET_PATH=$TET_INSTALL_PATH/tetware-simulator 
export TET_TARGET_PATH=$TET_INSTALL_PATH/tetware-target
export PATH=$TET_TARGET_PATH/bin:$PATH
export LD_LIBRARY_PATH=$TET_TARGET_PATH/lib/tet3:$LD_LIBRARY_PATH
export TET_ROOT=$TET_TARGET_PATH
