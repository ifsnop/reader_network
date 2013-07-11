#!/bin/bash

#clean build
rm libs/*.a 2> /dev/null
rm obj/*.o 2> /dev/null

rm bin/client* 2> /dev/null
rm bin/reader* 2> /dev/null
rm bin/cleanast* 2> /dev/null
rm bin/subscriber* 2> /dev/null

rm -rf 3rdparty/tmp 2> /dev/null

exit 0


 