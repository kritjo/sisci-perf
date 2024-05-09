#!/bin/bash

echo "Building the project"
./build.sh || exit 1

NODE_ID=$(./build/tools/print_node_id)

if [ "$NODE_ID" == "" ]; then
    echo "NODE_ID is not set. Exiting."
    exit 1
fi

echo "The current node (initiator) has node id: |$NODE_ID|"

if [ "$PEER_HOSTNAME" == "" ]; then
    echo "PEER_HOSTNAME is not set. Enter at least one peer hostname, multiple hostnames can be separated by space:"
    read PEER_HOSTNAME
fi

PEER_HOSTNAMES=($(echo "$PEER_HOSTNAME" | tr ' ' '\n'))

PEER_COUNT=${#PEER_HOSTNAMES[@]}

for ITER in $(seq 0 $(expr $PEER_COUNT - 1))
do
    PEER_NODE_IDS[$ITER]=$(./build/tools/print_node_id_from_hostname ${PEER_HOSTNAMES[$ITER]})
done

for ITER in $(seq 0 $(expr $PEER_COUNT - 1))
do
    echo "The peer node with hostname |${PEER_HOSTNAMES[$ITER]}| has node id: |${PEER_NODE_IDS[$ITER]}|"
    if [ "${PEER_NODE_IDS[$ITER]}" == "" ]; then
        echo "The node id for peer node with hostname |${PEER_HOSTNAMES[$ITER]}| is not set. Exiting."
        exit 1
    fi
    ITER=$(expr $ITER + 1)
done

./build/initiator/initiator_main $PEER_COUNT ${PEER_NODE_IDS[@]} &
INITIATOR_PID=$!

for ITER in $(seq 0 $(expr $PEER_COUNT - 1))
do
    stdbuf -oL -eL ssh ${PEER_HOSTNAMES[$ITER]} "cd $PWD && stdbuf -oL -eL ./build/peer/peer_main $NODE_ID" &
    PEER_PIDS[$ITER]=$!
done

# If we get a signal, kill all the peers and the initiator
trap "kill $INITIATOR_PID; kill ${PEER_PIDS[@]}; exit 1" INT TERM TSTP

wait ${PEER_PIDS[@]}