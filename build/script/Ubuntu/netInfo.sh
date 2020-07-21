#!/bin/bash

# "Usage:"
# "$0 [adapter name] [function]"
# "where function is following:"
# "\tM getAdapterNameList"
# "\tN getAdapterNameListEx"
# "\tL getNetworkCardList"
# "\tb getWiredSpeedMbpsTemp"
# "\t0 getWirelessAdapterNameList"
# "\t1 getWirelessSpeedMbps"
# "\t2 getWirelessStatus"
# "\t3 getRecvBytes"
# "\t4 getSendBytes"
# "\t5 getNetStatus"
# Example:
# sudo ./netInfo.sh none M
# sudo ./netInfo.sh none N
# sudo ./netInfo.sh none L
# sudo ./netInfo.sh ens33 b
# sudo ./netInfo.sh none 0
# sudo ./netInfo.sh ens33 1
# sudo ./netInfo.sh ens33 3
# sudo ./netInfo.sh ens33 4
# sudo ./netInfo.sh ens33 5


adapterName=$1

case "$2" in

	'M')
#adapterNameList=`ifconfig | grep "Link encap" | grep -v lo | awk '{print $1}'`
adapterNameList=`cat /proc/net/dev | grep ":" | grep -v lo | awk -F":" '{print $1}' | sed 's/^\s*//g'`
echo "$adapterNameList" | tee out.txt;;

	'N')
adapterNameListEx=`ifconfig | egrep '^[[:alpha:]].+' | grep -v lo | awk '{print $1}'`
echo "$adapterNameListEx" | tee out.txt;;

	'L')
# type lspci > /dev/null 2>&1
# if [ $? == 0 ] && [ -s /etc/udev/rules.d/70-persistent-net.rules ];then
# netpciID=`lspci | grep -i net | awk '{print $1}'`
# #echo $netpciID>>out.txt
# echo "`cat /etc/udev/rules.d/70-persistent-net.rules | grep -A 1 "${netpciID}" | grep -v "${netpciID}" | awk -F"," '{print $8}' | awk -F"=" '{print $2}' | sed -r 's/.*"(.*)".*/\1/' | sed '/^$/d'`"  | tee out.txt
# echo "`lspci | grep -i net | awk -F":" '{print $3}' | sed 's/^\s*//g'`" | tee out.txt
# else
# echo "default">>out.txt
# fi;;
echo "default">>out.txt;;

	'C')
MACList=`ifconfig | grep HWaddr | awk -F" " '{print $5}' | sort -k2n | uniq`                   
echo "$MACList" | tee out.txt;;
 
	'a')
wiredNetStatus=`ethtool ${adapterName} | grep "Link detected" | awk -F":" '{print $2}'`
echo "${wiredNetStatus}" | tee out.txt;;

	'b')
#wiredNetSpeedMbpsTemp=`mii-tool 2>/dev/null |grep ${adapterName} | awk '{print $3}'`
#echo "$wiredNetSpeedMbpsTemp" | tee out.txt;;
wiredNetSpeedMbpsTemp=`ethtool ${adapterName} | grep Speed | awk -F":" '{print $2}'`
echo "$wiredNetSpeedMbpsTemp" | tee out.txt;;
 
	'0' )
wirelessAdapter=`iwconfig 2>/dev/null | grep "802.11" | awk '{print $1}'`
echo "${wirelessAdapter}" | tee out.txt;;

	'1' )
wirelessNetSpeedMbpsTemp=`iwconfig 2>/dev/null | grep -A 2 ${adapterName} | grep "Bit Rate" | awk -F"=" '{print $2}' | awk '{print $1}'`
echo "${wirelessNetSpeedMbpsTemp}" | tee out.txt;;

	'2' )
#ESSID or Nickname
#Access Point
wirelessNetStatus=`iwconfig 2>/dev/null | grep ${adapterName} | awk -F":" '{print $2}' | sed 's/[[:space:]]*$//'`
echo "${wirelessNetStatus}" | tee out.txt;;

	'3' )
recvBytes=`cat /proc/net/dev | grep ${adapterName} | awk -F":" '{print $2}' | awk -F" " '{print $1}'`
echo "${recvBytes}" | tee out.txt;;

	'4' )
sendBytes=`cat /proc/net/dev | grep ${adapterName} | awk -F":" '{print $2}' | awk -F" " '{print $9}'`
echo "${sendBytes}" | tee out.txt;;

	'5' )
#both wired or wireless	
netStatus=`ifconfig ${adapterName}| grep "\binet\b"`
echo "${netStatus}" | tee out.txt;;

	*)
        echo "other cases"
esac


