#!/bin/bash

DEVICE_ID="00000001-0000-0000-0000-000bab8cbdd9"
USER="4dc407da-d2b1-4075-941b-be3d3b33488f:964da61b-58d9-47bd-845c-ca672cd9f5ce"
PASSWORD="dno12hk0b6d984puhc39orpuo9"
HOST="40.81.30.124"

PkgType="UbuntuWISEAgentSetup"
# 1: highest, 2: increment
UPG_MODE="1"
# 1: download, 2: deploy, 15: all
ACTION_TYPE="1"

CUR_TIME=$(date +%s)
START_TIME=$(echo "${CUR_TIME}+10" | bc | xargs -I{} -- date --date="@{}" "+%H:%M:%S")
END_TIME=$(echo "${CUR_TIME}+1800" | bc | xargs -I{} -- date --date="@{}" "+%H:%M:%S")
SEND_TS="${CUR_TIME}000"

MESSAGE="{\"Data\":{\"ScheInfo\":{\"PkgOwnerId\":2,\"PkgType\":\"${PkgType}\",\"ActType\":${ACTION_TYPE},\"UpgMode\":${UPG_MODE},\"ScheType\":4,\"StartTime\":\"${START_TIME}\",\"EndTime\":\"${END_TIME}\",\"optGid\":2},\"DevID\":\"${DEVICE_ID}\",\"CmdID\":109},\"SendTS\":${SEND_TS}}"
mosquitto_pub -u ${USER} -P ${PASSWORD} -h ${HOST} -t "/wisepaas/general/ota/${DEVICE_ID}/otareq" -m "${MESSAGE}"

