#!/bin/bash

# Custom Section
INS_DIR=/usr/local/EdgeSense/${PROJECT_NAME}
CONF_FILE_NAME=HDD_PMQ.ini
CONF_FILE_PATH=${INS_DIR}/${CONF_FILE_NAME}

function install_dependency ()
{
    sudo apt -y install net-tools iputils-ping libssl-dev gawk sed libxml2 libmosquitto1 sqlite3
    return
}

function backup_config ()
{
    rm -f /tmp/${CONF_FILE_NAME} || exit 1
    if [ -f $CONF_FILE_PATH ]; then
        echo "backup $CONF_FILE_PATH to /tmp"
        cp -f $CONF_FILE_PATH /tmp || exit 1
    fi
    return
}

function restore_config ()
{
    # Custom Section
    if [ -f /tmp/$CONF_FILE_NAME ]; then
        echo "restore $CONF_FILE_NAME to $CONF_FILE_PATH"
        mv -f /tmp/$CONF_FILE_NAME $CONF_FILE_PATH || exit 1
    fi
    return
}

function install_others ()
{
    return
}

