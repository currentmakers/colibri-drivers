#!/bin/bash
#

cd cmake-build-debug  
for IO in io_aic  io_aiv  io_aqv  io_dio1  io_empty  io_pid1  io_ssr  rgb_blink ; do 
    xxd -i -n $IO $IO.bin >$IO.c
done

