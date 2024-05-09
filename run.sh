echo "Building the project"
./build.sh

NODE_ID=$(./build/tools/print_node_id)

echo "The current node (initiator) has node id: |$NODE_ID|"

if [ "$PEER_HOSTNAME" == "" ]; then
    echo "PEER_HOSTNAME is not set. Enter the peer hostname:"
    read PEER_HOSTNAME
fi

PEER_NODE_ID=$(./build/tools/print_node_id_from_hostname $PEER_HOSTNAME)

echo "The peer node has node id: |$PEER_NODE_ID|"