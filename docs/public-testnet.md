
Purpose of this document
------------------------

This document is internal notes for the Graphene developers on how to
start up a new public testnet.

Updating previous testnet genesis
---------------------------------

Starting genesis file:

    8ff86468d7da87ae73a0a1fdc8adbf6353c427aacba7e27d33772afddabb5b07  genesis/aug-17-test-genesis-c.json

Run following commands:

    cp genesis/aug-17-test-genesis-c.json genesis/gnew1.json
    sed -i -e 's/"block_interval": 1/"block_interval": 5/' genesis/gnew1.json
    sed -i -e 's/"\(block_signing_key\|owner\|owner_key\|active_key\)": "BTS/"\1": "GPH/' genesis/gnew1.json

    PREFIX=aaaa
    programs/genesis_util/genesis_update -g genesis/gnew1.json -o genesis/gnew2.json --dev-account-count=1000 --dev-balance-count=200 --dev-key-prefix "$PREFIX"

Starting the network
--------------------

Create data dir:
    programs/witness_node/witness_node --genesis-timestamp 10 --genesis-json genesis/gnew2.json --enable-stale-production --data-dir data/gnew

Display witness keys and ID's for copy-pasting:
    programs/genesis_util/get_dev_key "$PREFIX" wit-block-signing-0:101 | python3 -c 'import json; import sys; print("\n".join("""private-key = ["{public_key}", "{private_key}"]""".format(**d) for d in json.load(sys.stdin)))' | sh -c 'cat >> data/gnew/config.ini'
    python3 -c 'print("\n".join("witness-id = \"1.6.{}\"".format(i) for i in range(1, 102)))' | sh -c 'cat >> data/gnew/config.ini'

Also set p2p and rpc endpoints.

NB you have to go into config.ini and move them around to the main section, it does not play nicely with appending because of sections.

Witness node startup
--------------------

We need to blow away the previous blockchain so we can rewrite the genesis timestamp:

    rm -Rf data/gnew/blockchain

Open up a new file in a text editor to take some notes.
Run the witness node like this:

    programs/witness_node/witness_node --genesis-timestamp 10 --genesis-json genesis/gnew2.json --enable-stale-production --data-dir data/gnew

You should copy-paste a line like this into your notes:

    Used genesis timestamp:  2015-08-31T18:36:45 (PLEASE RECORD THIS)

You should also copy-paste the printed chain ID since you will
need it in the next step.

Writing final genesis
---------------------

The genesis timestamp needs to be pasted into `genesis.json` file in the
`initial_timestamp` field.  If you do not do this, nodes using the
genesis will have a different chain ID and be unable to connect.
(Essentially the `--genesis-timestamp` option in the previous step tells the node
to overwrite the time in the file with a timestamp after startup.)

Running the wallet
------------------

    programs/cli_wallet/cli_wallet --wallet-file wallet/mywallet.json --server-rpc-endpoint ws://127.0.0.1:8091 -u abc -p xyz --chain-id 58903336bc82c0baa7ad0a0a0e12f4ecaff6a0e3826c4e9fb3b79a5034d69c17

    programs/genesis_util/get_dev_key "$PREFIX" active-0
    import_key devacct0 5KAceDNGYcBJrwLMeL5gQ3xjrEB2fy5ajFoEUBmt8nPyF8ruoSi
    programs/genesis_util/get_dev_key "$SECRET" wit-active-0
    import_key init0 5K5dAU3Xjjm3y6k6qt8V3pbJVc54w1yhh1GrLZD5wUYLgDaabnm
    programs/genesis_util/get_dev_key "$PREFIX" balance-0
    import_balance devacct0 ["5KaUCUiwMCBmCvnj9n8z2vwcmaoAaLdgaRmnZ1bB2ZTLzJqRJmr"]

Embedding genesis (optional)
----------------------------

See https://github.com/cryptonomex/graphene/wiki/egenesis for instructions
embedding the new genesis file in binaries.
