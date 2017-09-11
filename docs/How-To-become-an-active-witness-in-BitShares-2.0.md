This document serves as an introduction on how to become an actively block
producing witness in in the BitShares2.0 network. Please note that there
currently is no public testnet available, hence, this howto will fail at the
last few steps. However, we feel that we should inform interested parties about
how to prepare their machines for participation as witness as soon as possible.

We will have to import an existing account from the BitShares 0.9 network and
add some initial funds for the witness registration fee. After that, we will
create, configure and run a witness node.

## Preparations in BitShares 0.9 network

### Extracting an account from BitShares 0.9
To create a new account, you will need to start with an existing account with
some of the BTS asset that will pay the transaction fee registering your new
witness. Get your `<wif>` key from BitShares 0.9 via

    BitShares0.9: >>> wallet_dump_account_private_key <accountname> "owner_key"
    "5....."  # the <owner wif key>

### Extracting balances from BitShares 0.9
The key we have extracted previously only gives access to the registered name.
Hence, none of the accounts in the genesis block have balances in them. In the
first BitShares network, accounts were less tightly coupled to balances.
Balances were associated with public keys, and an account could have hundreds of
public keys with balances (or, conversely, public keys with balances could exist
without any account associated with them). 

In order to get a witness registered we need to import approximately $120 worth of
BTS into the BitShares 2.0 client later.

#### Manually extracting private keys (most secure way)
We can extract the required private keys that hold funds this way. First we get
all balance ids from an account via:

    BitShares0.9: >>> wallet_account_balance_ids <accountname>
    [[
    "xeroc",[
      "BTSAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
      "BTSBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB",
          ...
    ]
      ]
    ]

Each of these balances can be investigated via:

    BitShares0.9: >>> blockchain_get_balance BTSAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
    ....
    "asset_id": 0,                                     <- asset_id (0: BTS)
    "data": {
      "owner": "BTSOOOOOOOOOOOOOOOOOOOOOOOOOOOOOWNER", <- address
          ...
      "balance": 0,                                        <- balance
      ...

The required part (the owner of the balance) is denoted as `owner`.
Pick one or more address for BTS balances and dump the corresponding private key(s) with:

    BitShares0.9: >>> wallet_dump_private_key BTSOOOOOOOOOOOOOOOOOOOOOOOOOOOOOWNER
    "5......." # the <balance wif key>

Note: Make sure to secure these private keys, as they are unencrypted and give
access to the funds in the BitShares 0.9 network. You may loose your money if
you are not an secure computer!

#### Scripted extraction with Python (requires code audit)
The following paragraphs will give an alternative (easier) way to dump the
relevant private keys using a python script. If you are comfortable with the
description above, you can safely skip the subsequent paragraph.

A Python script located at
[github](https://github.com/xeroc/bitshares-pytools/blob/master/tools/getbalancekeys.py)
may help you to retrieve private keys for your balances.
You need to modify the first few lines of the script in order to get a
connection to your BitShares daemon.

    $ edit getbalancekeys.py
        [...]
    config.url    = "http://10.0.0.16:19988/rpc"
    config.user   = 'rpc-user'
    config.passwd = 'rpc-password'
    config.wallet = "default"
        [...]

If you don't know what to do with these, you certainly shouldn't run a witness
just now. Instead, read about [RPC and the
API](http://wiki.bitshares.org/index.php/BitShares/API) of BitShares.
If you set up everything correctly, you may just run the python script and get
the private keys associated to a given account name and the correspoinding
balance:

    $ python getbalancekeys.py
    accountA   5xxxxxxxxxxxxxxxxxxxxxxxxxx<owner wif key>           2750.00000 BTS        
    accountB   5xxxxxxxxxxxxxxxxxxxxxxxxxx<owner wif key>          11246.00000 BTS
    accountB   5xxxxxxxxxxxxxxxxxxxxxxxxxx<owner wif key>          30000.00000 BROWNIE.PTS
    accountB   5xxxxxxxxxxxxxxxxxxxxxxxxxx<owner wif key>            300.00000 USE
    accountB   5xxxxxxxxxxxxxxxxxxxxxxxxxx<owner wif key>          65744.00000 NOTE
    accountC   5xxxxxxxxxxxxxxxxxxxxxxxxxx<owner wif key>              3.00000 GOLD
    [...]
    accountA's owner key 5xxxxxxxxxxxxxxxxxxxxxxxxxx<balance wif key>  # 
    accountB's owner key 5xxxxxxxxxxxxxxxxxxxxxxxxxx<balance wif key>  # 
    accountC's owner key 5xxxxxxxxxxxxxxxxxxxxxxxxxx<balance wif key>  # 


You will only need BTS balances and the one of your account owner keys in order
to become a witness.

## BitShares 2.0 network (or Graphene testnet)

We now have everything prepared to

* import an existing account into the testnet
* redeem funds to register a witness in the testnet

and we will now continue with the following steps:

* create a wallet for the testnet
* import an account and funds
* upgrade our account to a lifetime member
* register a new witness
* upvote the witness with our funds
* sign blocks

From this point on, we will no longer require interaction with BitShares 0.9.

### Download the genesis block (only for testnet)

For the testnet we need to download the proper genesis block. Eventually, the
genesis block will be part of the client so that this step will not be required
for the real network later. The genesis block can be downloaded (here)[https://drive.google.com/open?id=0B_GVo0GoC_v_S3lPOWlUbFJFWTQ].

### Run the witness as a node in the network
We first run the witness node without block production and connect it to the P2P
network with the following command:

    $ programs/witness_node/witness_node -s 104.200.28.117:61705 --rpc-endpoint 127.0.0.1:8090 --genesis-json aug-14-test-genesis.json

The address `104.200.28.117` is one of the public seed nodes.

### Creating a wallet
We now open up the cli_wallet and connect to our plain and stupid witness node:

    $ programs/cli_wallet/cli_wallet -s ws://127.0.0.1:8090

First thing to do is setting up a password for the newly created wallet prior to
importing any private keys:

    new >>> set_password <password>
    null
    locked >>> unlock <password>
    null
    unlocked >>>

Wallet creation is now done.

### Basic Account Management
We can import the account name (owner key) and the balance containing keys into
BitShares 2.0:

    unlocked >>> import_key <accountname> <owner wif key>
    true
    unlocked >>> import_balance <accountname> [<balance wif key>] true
    [a transaction in json format]
    unlocked >>> list_my_accounts
    [{
    "id": "1.2.15",
    ...
    "name": <accountname>,
    ...
    ]
    unlocked >>> list_account_balances <accountname>
    XXXXXXX BTS

Note: Make sure to put the []-brackets around the private key, since the import
method takes an array of keys.  

In case your account's owner key is different from its active key, make sure you import it into BitShares 2.0 as well.

Since only lifetime members can become witnesses, you must first upgrade to a
lifetime member. This step costs the lifetime-upgrade fee which will eventually
cost about $100

    unlocked >>> upgrade_account <accountname> true
    [a transaction in json format]

### Becoming a Witness
To become a witness and be able to produce blocks, you first need to create a
witness object that can be voted in.

Note: If you want to experiment with things that require voting, be aware that
votes are only tallied once per day at the maintenance interval.
`get_dynamic_global_properties` tells us when that will be in
`next_maintenance_time`. Once the next maintenance interval passes, run
`get_global_properties` again and you should see that your new witness has been
voted in.  

Before we get started, we can see the current list of witnesses voted in, which
will simply be the ten default witnesses:

    unlocked >>> get_global_properties
    ...
      "active_witnesses": [
    "1.6.0",
    "1.6.1",
    "1.6.2",
    "1.6.3",
    "1.6.4",
    "1.6.5",
    "1.6.6",
    "1.6.7",
    "1.6.8",
    "1.6.9"
      ],
    ...

    unlocked >>> create_witness <accountname> "url-to-proposal" true
    {
      "ref_block_num": 139,
      "ref_block_prefix": 3692461913,
      "relative_expiration": 3,
      "operations": [[
      21,{
        "fee": {
          "amount": 0,
          "asset_id": "1.3.0"
        },
        "witness_account": "1.2.16",
        "url": "url-to-proposal",
        "block_signing_key": "PUBLIC KEY",
        "initial_secret": "00000000000000000000000000000000000000000000000000000000"
      }
    ]
      ],
      "signatures": [
      "1f2ad5597af2ac4bf7a50f1eef2db49c9c0f7616718776624c2c09a2dd72a0c53a26e8c2bc928f783624c4632924330fc03f08345c8f40b9790efa2e4157184a37"
      ]
    }

Our witness is registered, but it can't produce blocks because nobody has voted
it in.  You can see the current list of active witnesses with
`get_global_properties`:

    unlocked >>> get_global_properties
    {
      "active_witnesses": [
    "1.6.0",
    "1.6.1",
    "1.6.2",
    "1.6.3",
    "1.6.4",
    "1.6.5",
    "1.6.7",
    "1.6.8",
    "1.6.9"
      ],
      ...

Now, we should vote our witness in.  Vote all of the shares in both
`<accountname>` and `nathan` in favor of your new witness.

    unlocked >>> vote_for_witness <accountname> <accountname> true true
    [a transaction in json format]

Now we wait until the next maintenance interval.

Get the witness object using `get_witness` and take note of two things. The
`id` is displayed in `get_global_properties` when the witness is voted in, and
we will need it on the `witness_node` command line to produce blocks.  We'll
also need the public `signing_key` so we can look up the correspoinding private
key.

Once we have that, run `dump_private_keys` which lists the public-key 
private-key pairs to find the private key.

Warning: `dump_private_keys` will display your keys unencrypted on the
terminal, don't do this with someone looking over your shoulder.

    unlocked >>> get_witness <accountname>
    {
      [...]
      "id": "1.6.10",
      "signing_key": "GPH7vQ7GmRSJfDHxKdBmWMeDMFENpmHWKn99J457BNApiX1T5TNM8",
      [...]
    }

The `id` and the `signing_key` are the two important parameters, here. Let's get
the private key for that signing key with:

    unlocked >>> dump_private_keys
    [[
      ...
      ],[
    "GPH7vQ7GmRSJfDHxKdBmWMeDMFENpmHWKn99J457BNApiX1T5TNM8",
    "5JGi7DM7J8fSTizZ4D9roNgd8dUc5pirUe9taxYCUUsnvQ4zCaQ"
      ]
    ]

Now we need to start the witness, so shut down the wallet (ctrl-d),  and shut
down the witness (ctrl-c).  Re-launch the witness, now mentioning the new
witness 1.6.10 and its keypair:

    ./witness_node --rpc-endpoint=127.0.0.1:8090 --witness-id '"1.6.10"' --private-key '["GPH7vQ7GmRSJfDHxKdBmWMeDMFENpmHWKn99J457BNApiX1T5TNM8", "5JGi7DM7J8fSTizZ4D9roNgd8dUc5pirUe9taxYCUUsnvQ4zCaQ"]' --genesis-json aug-14-test-genesis.json -s 104.200.28.117:61705

Alternatively, you can also add this line into yout config.ini:

    witness-id = "1.6.10"
    private-key = ["GPH7vQ7GmRSJfDHxKdBmWMeDMFENpmHWKn99J457BNApiX1T5TNM8","5JGi7DM7J8fSTizZ4D9roNgd8dUc5pirUe9taxYCUUsnvQ4zCaQ"]

Note: Make sure to use YOUR public/private keys instead of the once given above!

If you monitor the output of the `witness_node`, you should see it generate 
blocks signed by your witness:

    Witness 1.6.10 production slot has arrived; generating a block now...
    Generated block #367 with timestamp 2015-07-05T20:46:30 at time 2015-07-05T20:46:30
