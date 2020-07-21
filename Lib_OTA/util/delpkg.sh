#!/bin/bash

DEVICE_ID="00000001-0000-0000-0000-0242FB508FEF"
USER="alex.shao@advantech.com.tw_created"
PASSWORD="ab987aa11e22f02ffb72b693a0b0ea4b"
HOST="172.22.13.10"

PKG_NAMES="CreateDir-v1.0.0.2-a9d6612910a37dee7be162ca5bd985ff.zip"
PKG_OWNER_ID=2

CUR_TIME=$(date +%s)
SEND_TS="${CUR_TIME}000"

MESSAGE="{\"Data\":{\"TaskInfo\":{\"PkgOwnerId\":${PKG_OWNER_ID},\"PkgNames\":[\"${PKG_NAMES}\"]},\"DevID\":\"${DEVICE_ID}\",\"CmdID\":121},\"SendTS\":${SEND_TS}}"
mosquitto_pub -u "${USER}" -P ${PASSWORD} -h ${HOST} -t "/wisepaas/general/ota/${DEVICE_ID}/otareq" -m "${MESSAGE}"

