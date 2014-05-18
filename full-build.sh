
#!/bin/bash

# this needs to be synced with Jamfile (sucky I know..)
NEWSFLASH_INSTALL_DBG=`pwd`/bin/debug
NEWSFLASH_INSTALL_REL=`pwd`/bin/release


while true; do
    read -p "Going to remove old binaries. c)ontinue, q)uit: " yc
    case $yc in 
        [Cc]* ) break;;
        [Qq]* ) exit;;
        * ) echo "Please answer c)ontinue or q)uit." ;;
    esac
done

rm -rf $NEWSFLASH_INSTALL_REL
rm -rf $NEWSFLASH_INSTALL_DBG
mkdir $NEWSFLASH_INSTALL_DBG
mkdir $NEWSFLASH_INSTALL_REL

echo "Building newsflash (debug)"
bjam debug -j8

echo "Building newsflash (release)"
bjam release -j8


# build third parties
echo "Building 3rd party tools"

mkdir $NEWSFLASH_INSTALL_REL/tools/
mkdir $NEWSFLASH_INSTALL_DBG/tools/

echo "Building unrar"
cd unrar 
rm unrar
make clean
make 2>&1> /dev/null
cp -v unrar $NEWSFLASH_INSTALL_DBG/tools/
cp -v unrar $NEWSFLASH_INSTALL_REL/tools/
cp -v LICENSE_UNRAR.txt $NEWSFLASH_INSTALL_REL/tools/
cp -v LICENSE_UNRAR.txt $NEWSFLASH_INSTALL_DBG/tools/
cd ..

echo "Building par2"
cd par2cmdline
./configure 2>&1> /dev/null
make 2>&1> /dev/null
cp -v par2 $NEWSFLASH_INSTALL_DBG/tools/
cp -v par2 $NEWSFLASH_INSTALL_REL/tools/
cp -v LICENSE_PAR2.txt $NEWSFLASH_INSTALL_REL/tools/
cp -v LICENSE_PAR2.txt $NEWSFLASH_INSTALL_DBG/tools/
cd ..
