# SISCI Perf
A framework for testing Dolphin Interconnect Solutions NTB's performance using the SISCI API.

## Usage
Create a file named `.callback.build`, this should contain any bash code that you want to be run before the building step. An example of this can be loading gcc modules.

Start by enabling the features you want to profile in `initiator/initiator_main.h` - by default, all safe are enabled.
Some experiment options can be adjusted in their respective header files under `initiator/`.

**Prerequisites**
- All nodes that should be a part of the experiment (at least two) has to have a shared disk mounted at the same location, as most shared clusters have these days.
- This repository has to be cloned inside of the shared disk.
- The working directory has to be the inside the cloned repository's root.
- Copy your SSH key from the node you are running the script from (the _initiator_) to all others (the _peers_).

```bash
./run.sh
```

Run the `run.sh` script from the initiator node. It will prompt you for other hostnames for the peers to be used in the experiments, or set the `PEER_HOSTNAME` environment variable.
The source code will be automatically compiled and the chosen experiments run accross the cluster.

## Writing custom tests
1. Create a pair of `<experiment_name>.c` and `<experiment_name>.h` files, with at least a declaration and definition for the main
experiment case.
2. Be sure to use the `init_timer()` function in the main experiment runner function and use (or create a new) executor function in
`common_read_write_functions.h`. If you create a new executor, make sure to follow the style of the exising ones (start by zeroing the operations global and enter a while loop checking for the timer_expired global).

## The Peer node
Acts as a server for the Initiator. Each peer has one order data interrupt from which the Initiator can order segments, interrupts,
and similar functionality that the experiments need. The Peer node replies to the Initiator through the Initiator's delivery data
interrupt. See the `lib/protocol.h` header for the structs.
