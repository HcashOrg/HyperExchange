
Chain-locked transactions
-------------------------

Add a chain ID to the signature.  This ensures that a transaction
without TaPoS cannot migrate between Graphene-based chains, and
reduces the ability for the transaction to migrate to a different
protocol (e.g. if your Graphene key is also a Bitcoin key, then a
Graphene transaction which is parseable as a Bitcoin transaction
might be able to migrate to the Bitcoin chain).

Wallet functionality
--------------------

The `cli_wallet` stores the `chain_id` in the wallet file.  Thus a
wallet file can only be used with a single chain.  When creating a
new wallet file, the `chain_id` can be specified on the command line;
if none is specified, the embedded `chain_id` will be used by default.

When connecting to an API server, the `cli_wallet` will check the
chain ID provided by the server matches the client, and disconnect
immediately if the check fails.

Light wallet functionality
--------------------------

At present, the light wallet simply uses the chain ID specified by its backing full node to sign transactions.
