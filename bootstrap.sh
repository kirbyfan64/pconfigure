#!/bin/bash

SOURCE_PATH="$1"
BOOTSTRAP_DIR=bootstrap_bin

make distclean >& /dev/null || true
mkdir -p $BOOTSTRAP_DIR

# Manually builds some of the utilities
gcc --std=gnu99 -Wall -Werror -Wno-trampolines -g \
    `find "$SOURCE_PATH"src/pconfigure/ -iname "*.c"` \
    -L`llvm-config --libdir` \
    -Wl,-R`llvm-config --libdir` \
    -I`llvm-config --includedir` \
    `llvm-config --libs core` \
    -lclang \
    `pkg-config talloc --libs` \
    -DPCONFIGURE_VERSION=\"bootstrap\" \
    -o "$BOOTSTRAP_DIR/pconfigure" || exit $?

gcc --std=gnu99 `find "$SOURCE_PATH"src/pbashc/ -iname "*.c"` \
    -o "$BOOTSTRAP_DIR/pbashc"

$BOOTSTRAP_DIR/pbashc "$SOURCE_PATH"src/pclean/main.bash \
    -o $BOOTSTRAP_DIR/pclean

$BOOTSTRAP_DIR/pbashc "$SOURCE_PATH"src/ppkg-config/main.bash \
    -o $BOOTSTRAP_DIR/ppkg-config

# Runs pconfigure in order to build itself
if [[ "$SOURCE_PATH" != "" ]]
then
    sp="--sourcepath $SOURCE_PATH"
else
    sp=""
fi
env PATH="$BOOTSTRAP_DIR:$PATH" $BOOTSTRAP_DIR/pconfigure $sp
err="$?"
if [[ "$err" != "0" ]]
then
    exit $err
fi

# Actually builds itself
env PATH="$BOOTSTRAP_DIR:$PATH" make || exit $?
env PATH="$BOOTSTRAP_DIR:$PATH" make all_install || exit $?

# Cleans up from the bootstrap process
rm -rf $BOOTSTRAP_DIR

# Informational messages to the user
prefix=`cat "$SOURCE_PATH"src/pconfigure/context.c | grep prefix | head -1 | cut -d \" -f 2`
echo "run 'make install' to install this to the system"
echo -e "\tby default it is installed into $prefix"

prefix=`cat "$SOURCE_PATH"Configfiles/{local,main} | grep PREFIX | head | cut -d '=' -f 2`
prefix=`echo $prefix`
if [[ "$prefix" != "" ]]
then
    echo -e "\t(you have it set to install to $prefix)"
fi
