#!/bin/bash

URL=ftp://admin:sa30Admin@edgesense4.wise-paas.com:2121/$1

DEVICE_ID="00000001-0000-0000-0000-000BAB8CBDDA"
USER="918399d3ad0c1b673a4f87b32d778264_created"
PASSWORD="1df723710d8fdfb577e4a0877ba922e5"
HOST="deviceon.wise-paas.com"

# 1:ftp, 2:azure, 3:s3c, 4:s3
PROTOCOL="1"
# is deploy?
DEPLOY="0"
PKG_OWNER_ID=2

CUR_TIME=$(date +%s)
SEND_TS="${CUR_TIME}000"
FILE=$(echo "${URL}" | awk -F "/" '{print $NF}')
URL_ENC=$(./otacodec -e ${URL} 2>&1)

MESSAGE="{\"Data\":{\"TaskInfo\":{\"PkgOwnerId\":${PKG_OWNER_ID},\"PkgType\":\"mytest\",\"URL\":\"${URL_ENC}\",\"PkgName\":\"${FILE}\",\"Protocal\":${PROTOCOL},\"Sercurity\":100,\"IsDp\":${DEPLOY},\"DlRetry\":3,\"DpRetry\":0,\"IsRB\":0},\"DevID\":\"${DEVICE_ID}\",\"CmdID\":101},\"SendTS\":${SEND_TS}}"
#MESSAGE="{\"Data\":{\"TaskInfo\":{\"PkgOwnerId\":${PKG_OWNER_ID},\"PkgType\":\"CreateDir\",\"URL\":\"${URL_ENC}\",\"PkgName\":\"${FILE}\",\"Protocal\":${PROTOCOL},\"Sercurity\":100,\"IsDp\":${DEPLOY},\"DlRetry\":3,\"DpRetry\":0,\"IsRB\":0},\"DevID\":\"${DEVICE_ID}\",\"CmdID\":101},\"SendTS\":${SEND_TS}}"

mosquitto_pub -u ${USER} -P ${PASSWORD} -h ${HOST} -t "/wisepaas/general/ota/${DEVICE_ID}/otareq" -m "${MESSAGE}"

