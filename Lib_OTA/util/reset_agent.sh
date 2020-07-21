#!/bin/bash
set -x

URL="https://deviceon.wise-paas.com/rmm/v1/iothub/credential"
IOTKEY=7863b90563810fcb9e45a03f7aa15c96
AGENT_INSTALLER='wise-agent-Ubuntu 18.04 x86_64-1.4.4.0.run'

cd /usr/local/AgentService
sudo ./uninstall.sh
cd

set -e

echo -e "n\n" | sudo -S ./"${AGENT_INSTALLER}"
sleep 5
sudo sed -i "s|<CredentialURL>.*|<CredentialURL>${URL}</CredentialURL>|" /usr/local/AgentService/agent_config.xml
sudo sed -i "s|<IoTKey.*|<IoTKey>${IOTKEY}</IoTKey>|" /usr/local/AgentService/agent_config.xml
sudo sed -i "s|<UserName.*|<UserName>${IOTKEY}</UserName>|" /usr/local/AgentService/agent_config.xml
echo -e "\n[LogClient]\nlog_level=5" | sudo tee -a /usr/local/AgentService/log.ini
sudo systemctl restart saagent

