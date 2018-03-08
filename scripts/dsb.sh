#!/bin/bash
#
#  programname 	:	dsb.sh alias do_simple_browse.sh
#  programmer   :       L.Pearl c/- Bruces' Bible Translation project
#  date written :	31/8/2017
#
#  this code needs to see how many 'kbserver' instances are on the LAN , cycle
#  through them to produce the 'serial' list of records returned
#

function do_LAN_browse 
{
    rm kbservice_file.dat 2> /dev/null
    rm kbs.dat 2> /dev/null
    avahi-browse -ltp _kbserver._tcp --resolve | grep -v '^+' | grep -v v6 | grep wlan > kbservice_file.dat
    #
    # if there is an ouput to use then ....
    #
    if [ -s kbservice_file.dat ]
    then
           # kbservice_file.dat looks like :_
           # =;wlan0;IPv4;amd64-mint18;_kbserver._tcp;local;amd64-mint18.local;192.168.0.2;3000;
           #
           # for kbs.dat output remove '.local.',amd remove the last comma :-
           # 192.168.0.3@@@toshiba-Mint18,192.168.0.2@@@amd64-mint18
           #
           awk -v ORS="," -F ';' '{ print $8,$7 }' kbservice_file.dat > kbs.dat
           sed -e 's/ /@@@/g' -e 's/.local//g' -e 's/.$//' kbs.dat > kbservice_file.dat
    fi      
} 
do_LAN_browse
exit 0
