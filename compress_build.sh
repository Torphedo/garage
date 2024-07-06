#!/bin/sh

compress() {
    if [ -f $1 ]; then
        # Silence strip and don't try to compress again if it fails.
        strip $1 &> /dev/null && upx -9 $1
    fi
}

if [ -z $1 ]; then
    echo "Please provide the build folder as an argument to the script."
    false
else
    compress "$1/garage"
    compress "$1/test/test"
    compress "$1/txt2h"

    true
fi

