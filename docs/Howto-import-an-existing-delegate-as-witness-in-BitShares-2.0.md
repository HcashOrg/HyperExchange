## Preparations in BitShares 0.9 network

We need to Extract the signing public and private key from BitShares 0.9.

Let's obtain the `<publickey>`:

    >>> get_account <delegatename>
    [...]
    Block Signing Key: <publickey>
    [...]

**Remark**: Public keys in the BitShares network have the prefix `BTS`. Hence, in the case of the Graphene testnet you should replace `BTS` by `GPH`.

and the corresponding `<wifkey>`:

    >>> wallet_dump_account_private_key <delegatename> signing_key
    "<wifkey>"

## BitShares 2.0 network (or Graphene testnet)

### Download the genesis block (only for testnet)

For the testnet we need to download the proper genesis block. Eventually, the
genesis block will be part of the client so that this step will not be required
for the real network later. The genesis block can be downloaded (here)[https://drive.google.com/open?id=0B_GVo0GoC_v_S3lPOWlUbFJFWTQ].

### Run the witness as a node in the network
We first run the witness node without block production and connect it to the P2P
network with the following command:

    $ programs/witness_node/witness_node -s 104.200.28.117:61705 --rpc-endpoint 127.0.0.1:8090 --genesis-json aug-14-test-genesis.json

The address `104.200.28.117` is one of the public seed nodes.

### Retreiving witness_id
We now open up the `cli_wallet` and connect to our plain and stupid witness node:

    $ programs/cli_wallet/cli_wallet -s ws://127.0.0.1:8090

The witness_id can be obtain from the blockchain:

    locked >>> get_witness <delegatename>

where `<delegatename` is the name of the account used as delegate in
BitShares0.9. This delegate is a "witness" in BitShares 2.0.

### Running a block producing witness

Now we need to start the witness, so shut down the wallet (ctrl-d),  and shut
down the witness (ctrl-c).  Re-launch the witness, now mentioning the new
witness 1.6.10 and its keypair:

    ./witness_node --rpc-endpoint=127.0.0.1:8090 \
                   --witness-id '"<witnessid>"' \
                   --private-key '["<publickey>", "<wifkey>"]' \
                   --genesis-json aug-14-test-genesis.json \
                   -s 104.200.28.117:61705

Alternatively, you can also add this line into yout config.ini:

    witness-id = "<witnessid>"
    private-key = ["<publickey>", "<wifkey>"]

If you monitor the output of the `witness_node`, you should see it generate 
blocks signed by your witness:

    Witness 1.6.10 production slot has arrived; generating a block now...
    Generated block #367 with timestamp 2015-07-05T20:46:30 at time 2015-07-05T20:46:30
