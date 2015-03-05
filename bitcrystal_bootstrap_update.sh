#!/bin/bash
echo start updating bootstrap.dat
echo just a momment please...
echo;
echo;
echo;
rootpath=$HOME
mypath=./
bitcrystalpath=${rootpath}'/.bitcrystal_v20'
if [ ! -d $bitcrystalpath ]; then
		mkdir $bitcrystalpath
		chmod 755 $bitcrystalpath -R
		chown $(whoami):$(whoami) $bitcrystalpath -R
fi
rm ${mypath}'bootstrap.tar.gz' 1> /dev/null 2> /dev/null
rm ${mypath}'bootstrap.dat' 1> /dev/null 2> /dev/null
wget http://bitcrystaldownload.demon-craft.de/bitcrystal_conf_update/bootstrap.tar.gz
chmod 755 ./bootstrap.tar.gz
tar xvzf ./bootstrap.tar.gz
rm $bitcrystalpath/bootstrap.dat 1> /dev/null 2> /dev/null
cp ./bootstrap.dat $bitcrystalpath
rm ./bootstrap.tar.gz 1> /dev/null 2> /dev/null
rm ./bootstrap.dat 1> /dev/null 2> /dev/null
chmod 755 $bitcrystalpath/bootstrap.dat
chown $(whoami):$(whoami) $bitcrystalpath/bootstrap.dat
