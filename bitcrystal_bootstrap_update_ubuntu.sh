#!/bin/bash
rootpath=$HOME
mypath=./
bitcrystalpath=${rootpath}'/.bitcrystal_v20'
OS="UBUNTU"
OS_VERSION="7.0"
PREFIX=""
if [[ $OS == "UBUNTU" ]]; then
        PREFIX="sudo"
fi

WHOAMI_ROOT=$($PREFIX id -u);
if [ $WHOAMI_ROOT -ne 0 ]; then
        echo "Are you running this script under root?"
        exit 0;
fi
$PREFIX echo start updating bootstrap.dat
$PREFIX echo just a momment please...
$PREFIX echo;
$PREFIX echo;
$PREFIX echo;
if [ ! -d $bitcrystalpath ]; then
		$PREFIX mkdir $bitcrystalpath
		$PREFIX chmod 755 $bitcrystalpath -R
		$PREFIX chown $(whoami):$(whoami) $bitcrystalpath -R
fi
$PREFIX rm ${mypath}'bootstrap.tar.gz' 1> /dev/null 2> /dev/null
$PREFIX rm ${mypath}'bootstrap.dat' 1> /dev/null 2> /dev/null
$PREFIX wget http://bitcrystaldownload.demon-craft.de/bitcrystal_conf_update/bootstrap.tar.gz
$PREFIX chmod 755 ./bootstrap.tar.gz
$PREFIX tar xvzf ./bootstrap.tar.gz
$PREFIX rm $bitcrystalpath/bootstrap.dat 1> /dev/null 2> /dev/null
$PREFIX cp ./bootstrap.dat $bitcrystalpath
$PREFIX rm ./bootstrap.tar.gz 1> /dev/null 2> /dev/null
$PREFIX rm ./bootstrap.dat 1> /dev/null 2> /dev/null
$PREFIX chmod 755 $bitcrystalpath/bootstrap.dat
$PREFIX chown $(whoami):$(whoami) $bitcrystalpath/bootstrap.dat
