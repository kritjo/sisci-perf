#!/bin/bash

echo "Building the project, see build.log for details"
./build.sh > build.log 2>&1 || { echo "Build failed, see build.log for details"; exit 1; }
echo "Project built successfully"

NODE_ID=$(./build/tools/print_node_id)

if [ "$NODE_ID" == "" ]; then
    echo "Could not get local node id. Exiting."
    exit 1
fi

echo "The current node (initiator) has node id: $NODE_ID"

if [ "$PEER_HOSTNAME" == "" ]; then
    echo -n "PEER_HOSTNAME is not set. Enter at least one peer hostname, multiple hostnames can be separated by space: "
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
    echo "The peer node with hostname ${PEER_HOSTNAMES[$ITER]} has node id: ${PEER_NODE_IDS[$ITER]}"
    if [ "${PEER_NODE_IDS[$ITER]}" == "" ]; then
        echo "Could not get the node id for peer node with hostname ${PEER_HOSTNAMES[$ITER]}. Exiting."
        exit 1
    fi
    ITER=$(expr $ITER + 1)
done

./build/initiator/initiator_main $PEER_COUNT ${PEER_NODE_IDS[@]} &
INITIATOR_PID=$!

for ITER in $(seq 0 $(expr $PEER_COUNT - 1))
do
    RAND_FILE=$(mktemp)
    stdbuf -oL -eL ssh ${PEER_HOSTNAMES[$ITER]} "stdbuf -oL -eL $PWD/build/peer/peer_main $NODE_ID & echo \$! > $RAND_FILE" &
    sleep 0.1
    scp ${PEER_HOSTNAMES[$ITER]}:$RAND_FILE $RAND_FILE > /dev/null
    PEER_PIDS[$ITER]=$(cat $RAND_FILE)
done

# Trap signals and forward them to the child processes
trap "kill $INITIATOR_PID" INT TERM TSTP

wait $INITIATOR_PID

for ITER in $(seq 0 $(expr $PEER_COUNT - 1))
do
  ssh ${PEER_HOSTNAMES[$ITER]} "kill -INT ${PEER_PIDS[$ITER]}"
done

sleep 1

for ITER in $(seq 0 $(expr $PEER_COUNT - 1))
do
  ssh ${PEER_HOSTNAMES[$ITER]} "kill -KILL ${PEER_PIDS[$ITER]} > /dev/null 2>&1"
done

echo "Exiting"