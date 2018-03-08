#!/bin/bash

#
function do_LAN_browse 
{
    rm *.dat 2> /dev/null
    dns-sd -B _kbserver._tcp > service_file.dat &
    sleep 2
    ps -ef | grep -v grep | grep "dns-sd -B" | awk '{print $2}' > PIDfile
    kill -9 `cat PIDfile` 2>/dev/null
    grep "Add   " service_file.dat | awk '{ if (!a[$7]++ ) print ;}' > KBlist.dat
    for i in `cat KBlist.dat`
    do
        dns-sd -L $i _kbserver._tcp local >> reached_host.dat &
    done
    sleep 2
    ps -ef | grep -v grep | grep "dns-sd -l" | awk '{print $2}' > PIDfile
    kill -9 `cat PIDfile` 2>/dev/null
    #grep reached reached_host.dat | awk '{ if (!a[$7]++ ) print ;}' | cut -d'.' -f1 >> reached_hosts.dat
    grep reached reached_host.dat | awk '{ if (!a[$7]++ ) print ;}' | awk '{ print $7 }' | cut -d'.' -f1 >> reached_hosts.dat
    for j in `cat reached_hosts.dat`
    do
        dns-sd -G v4 $j >> IPaddress_list.dat &
    done
    ps -ef | grep -v grep | grep "dns-sd -G" | awk '{print $2}' > PIDfile
    kill -9 `cat PIDfile` 2>/dev/null
    grep "Add   " IPaddress_list.dat | awk 'BEGIN {ORS=","}{print $6,$5}' > IPAddress_report_temp.dat
    sed 's/.local//g' IPAddress_report_temp.dat > IPAddress_report_temp1.dat
    sed 's/.Home//g' IPAddress_report_temp1.dat > IPAddress_report_temp.dat
    sed 's/ /@@@/g' IPAddress_report_temp.dat >> report.dat
    sleep 2
    ps -ef | grep -v grep | grep "dns-sd -G" | awk '{print $2}' > PIDfile
    kill -9 `cat PIDfile` 2> /dev/null
} 
do_LAN_browse
exit 0
