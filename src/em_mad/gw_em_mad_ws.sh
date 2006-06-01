#!/bin/bash

ulimit -c 15000
cd /tmp

if [ -z "${GLOBUS_LOCATION}" ]; then
    echo "Please, set GLOBUS_LOCATION variable."
    exit -1
fi

if [ -z "${GW_LOCATION}" ]; then
    echo "Please, set GW_LOCATION variable."
    exit -1
fi

. $GLOBUS_LOCATION/etc/globus-user-env.sh

. $GLOBUS_LOCATION/etc/globus-devel-env.sh 

export CLASSPATH=$CLASSPATH:$GW_LOCATION/bin

if [ -z "${GLOBUS_TCP_PORT_RANGE}" ]; then
    java -DGLOBUS_LOCATION=$GLOBUS_LOCATION -Djava.endorsed.dirs=$GLOBUS_LOCATION/endorsed GW_mad_ws
else
    java -DGLOBUS_LOCATION=$GLOBUS_LOCATION -Djava.endorsed.dirs=$GLOBUS_LOCATION/endorsed -DGLOBUS_TCP_PORT_RANGE=$GLOBUS_TCP_PORT_RANGE GW_mad_ws
fi
