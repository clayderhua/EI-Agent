PKG_NAME=UbuntuWISEAgentSetup-v1.3.999.0-6e2a607fb6034cf4d278b521e11b289a.zip	
TASK=2 # 1: download, 2: deploy
STATUS=5 # 5: SC_FINISHED
PKG_OWNER_ID=103

DEVICE_ID="00000001-0000-0000-0000-000BAB8CBDD9"
USER="918399d3ad0c1b673a4f87b32d778264_created"
PASSWORD="1df723710d8fdfb577e4a0877ba922e5"
HOST="deviceon.wise-paas.com"

CUR_TIME=$(date +%s)
SEND_TS="${CUR_TIME}000"

MESSAGE="{\"SendTS\":${SEND_TS},\"Data\":{\"CmdID\":104,\"DevID\":\"${DEVICE_ID}\",\"TaskStatus\":{\"PkgName\":\"${PKG_NAME}\",\"Type\":${TASK},\"Status\":${STATUS},\"Action\":1,\"RetryCnt\":0,\"PkgOwnerId\":${PKG_OWNER_ID}}}}"

mosquitto_pub -u ${USER} -P ${PASSWORD} -h ${HOST} -t "/wisepaas/general/ota/${DEVICE_ID}/otaack" -m "${MESSAGE}"
