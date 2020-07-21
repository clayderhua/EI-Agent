#!/bin/bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/AgentService
echo $LD_LIBRARY_PATH
./cagent -n
