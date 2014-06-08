#!/bin/bash

# this script takes care of *full* build.
# incremental builds are handled by Jamfiles

CURRENT=`pwd`
VERBOSE=""
VARIANT="debug"

for arg in "$@"
do
    case "$arg" in 
        "--debug" ) VARIANT="debug" ;;
        "--release" ) VARIANT="release" ;;
        "--verbose" ) VERBOSE="-v" ;;
    * ) echo -e "'$arg' is not a valid build argument\n" 
    exit 1;;
    esac
done

INSTALL=""
case "$VARIANT" in
    "debug") INSTALL=`pwd`"/dist_d" ;;
    "release") INSTALL=`pwd`"/dist" ;;
esac

echo -e "Build variant: $VARIANT"
echo -e "Build install: $INSTALL"

REMOVE_BUILD=true
while true; do
    read -p "Going to remove old binaries. c)ontinue, q)uit s)kip: " yc
    case $yc in 
        [Cc]* ) break;;
        [Qq]* ) exit;;
        [Ss]* ) REMOVE_BUILD=false break;;
        * ) echo "Please answer c)ontinue or q)uit." ;;
    esac
done

echo "Starting build..."

if [ $REMOVE_BUILD ]; then
    rm -rf $VERBOSE "$INSTALL"
fi

if [ ! -e "$INSTALL" ]; then
    mkdir $VERBOSE "$INSTALL"
fi


function makedir() {
    local dir=$1
    mkdir $VERBOSE "$INSTALL/$dir"
}

function copy() {
    local src=$1
    local dst=$2
    cp -dr $VERBOSE "$src" "$INSTALL/$dst"
    if [ $? -ne 0 ]; 
    then
       echo "Error copying file $src"
       exit 1
    fi
}

PYTHON="Python-2.5.1"

makedir "tools"
makedir "media"
makedir "plugins"
makedir "plugins-qt"
makedir "plugins-qt/imageformats"
makedir "$PYTHON"

# build third parties
cd "tools/unrar" 
if [ ! -e "unrar" ]
then
    echo "Building unrar"
    make clean
    make 2>&1> /dev/null
fi
copy "unrar" "tools/"
copy "LICENSE_UNRAR.txt" "tools/"
cd ..
cd ..

cd "tools/par2cmdline"
if [ ! -e "par2" ]
then
    echo "Building par2"
    ./configure 2>&1> /dev/null
    make 2>&1> /dev/null
fi
copy "par2" "tools/"
copy "LICENSE_PAR2.txt" "tools/"
cd ..
cd ..

cd "qjson"
if [ ! -e "lib/libqjson.so" ]
then
    echo "Building qjson"
    cmake -G "Unix Makefiles"
    make 
fi
copy "lib/libqjson.so" ""
copy "lib/libqjson.so.0" ""
copy "lib/libqjson.so.0.8.1" ""
copy "README.license" "QJSON_LICENSE.txt"
cd ..

cd zlib
bjam -j8 release
copy "lib/libzlib.so" ""
copy "README" "ZLIB_README.txt"
cd ..


cd python
bjam -j8 $VARIANT
copy "LICENSE.txt" "$PYTHON"
copy "NEWS.txt" "$PYTHON"
copy "README.txt" "$PYTHON"
copy "DLLs" "$PYTHON"
copy "Lib" "$PYTHON"
copy "$VARIANT/python" "$PYTHON"
copy "$VARIANT/libpython25.so" ""
cd ..

cd "tools/launcher"
bjam $VARIANT
copy "bin/launcher" "tools/"
cd ..
cd ..

bjam -j8 "plugins/womble" $VARIANT
bjam -j8 "app" $VARIANT


QT=`pwd`/../qt-4.8.2

# install qt plugins and libs
copy "$QT/lib/libQtCore.so" ""
copy "$QT/lib/libQtCore.so.4" ""
copy "$QT/lib/libQtCore.so.4.8.2" ""
copy "$QT/lib/libQtNetwork.so" ""
copy "$QT/lib/libQtNetwork.so.4" ""
copy "$QT/lib/libQtNetwork.so.4.8.2" ""
copy "$QT/lib/libQtGui.so" ""
copy "$QT/lib/libQtGui.so.4" ""
copy "$QT/lib/libQtGui.so.4.8.2" ""
copy "$QT/plugins/imageformats/libqgif.so" "plugins-qt/imageformats"
copy "$QT/plugins/imageformats/libqjpeg.so" "plugins-qt/imageformats"

copy "newsflash.sh" ""
copy "../boost_1_51_0/LICENSE_1_0.txt" "BOOST_LICENSE.txt"
copy "media/download.wav" "media"
copy "media/error.wav" "media"
copy "media/update.wav" "media"
copy "doc/README.txt" ""

copy "help/*" "help"

cd "$INSTALL"
chmod u+x "runpython.sh"
cd "$CURRENT"
