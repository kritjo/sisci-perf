# SISCI Perf
A framework for testing Dolphin Interconnect Solutions NTB's performance using the SISCI API.

## Usage
Start by enabling the features you want to profile in `initiator/initiator_main.h`.
Some experiment options can be adjusted in their respective header files under `initiator/`.

```bash
./run.sh
```

Run the `run.sh` script from the initiator node. It will prompt you for other hostnames for the peers to be used in the experiments.
The source code will be automatically compiled and distributed to the peers (for simplicity copy your SSH key from the initiator to
the peers).

## Writing custom tests
1. Create a pair of `<experiment_name>.c` and `<experiment_name>.h` files, with at least a declaration and definition for the main
experiment case.
2. Be sure to use the `init_timer()` function in the main experiment runner function and use (or create a new) executor function in
`common_read_write_functions.h`.

## The Peer node
Acts as a server for the Initiator. Each peer has one order data interrupt from which the Initiator can order segments, interrupts,
and similar functionality that the experiments need. The Peer node replies to the Initiator through the Initiator's delivery data
interrupt. See the `lib/protocol.h` header for the structs.
