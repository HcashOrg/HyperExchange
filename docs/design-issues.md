
Purpose
-------

This is a list of some design issues based on my (@theoreticalbts) review of White Rabbit.  Feel free to add or delete from this.

Sub-objects
-----------

EDIT:  Seems like this has already implemented as "ID spaces".  Keeping this information here so it can eventually be incorporated into a spec.

Several objects such as `delegate_vote_object` (keeping track of vote totals for a delegate) and `balances` (keeping track of balances for an account) are kept in their own object ID.  This is not necessary for chain semantics, rather it is optimizing the memory usage of the undo state.

I think it is better to avoid splitting out these sub-objects in the blockchain specification, instead have a separate "local object ID space" local to each node where sub-objects are kept.  So from the blockchain spec level, a create account operation says "create an account object including balances."  But from the blockchain implementation level, a create account operation says "create an account object in the blockchain ID space, then create a balance object in the local ID space, then put a link the blockchain object's balances data field pointing to the local object."  Changing the balance would then request an immutable handle to the account object, through which you could get the local ID, then request a mutable handle to the balances object.  Achieving the desired result of having a balance change only require the undo state to copy the balances instead of the whole account object.

The strength of implementing this with a local object ID space is that we can later split things out into sub-objects, or join sub-objects into their containing object, without a hardfork.  Since the local ID's are never exposed outside the local node's implementation of the blockchain.

Tx size optimizations for TaPoS validation
------------------------------------------

EDIT:  I (@theoreticalbts) have taken a crack at this, it compiles but is not yet tested.

We have this:

```cpp
    struct transaction 
    {
        fc::time_point_sec expiration;
        uint32_t           ref_block_num    = 0;
        uint32_t           ref_block_prefix = 0;
        vector<operation>  operations;
     };
```

First, we should simply include the full block hash at height `ref_block_num` in the computation of `digest()`, thus the tx will fail to validate unless the block ID at the given height is a match.  Allowing to completely eliminate `ref_block_prefix` and also avoiding the potential security holes from using only 32 bits in `ref_block_prefix` field.

Second, the `ref_block_num` itself can be reduced to 16 bits as follows:  Simply specify the low 16 bits of the reference block height, and only validate against the most recent block whose low 16 bits match.  A more recent block whose block height has the same low 16 bits as the reference block height cannot appear until 64K blocks after the reference block, in which case the transaction is over a day old and will fail due to being expired (unless we decrease block time below 2 seconds).

processed_transaction has no purpose
------------------------------------

The `processed_transaction` is included in a block.  This is too much data.  Since object ID's are assigned sequentially and deterministically, this is totally redundant information.  A single note specifying the first ID to be allocated in the block header would suffice, but even that much is redundant.  Once we have over a couple million objects, storing the object ID's will inflate all object creation ops by four bytes totally unnecessarily.

Block header does not support SPV
---------------------------------

We should be able to verify blockchain using headers only.  Thus we need to have hash of tx's in block header instead of `vector<processed_transaction>`.

Cache block hashes
------------------

We need some in-memory structure for a block that caches block and transaction hashes instead of recomputing it every time we want it (e.g. every transaction must compute reference block hash).  This means maybe we should have an in-memory block / tx type distinct from the on-the-wire block / tx type, which annotates it with extra information like the hash.  And maybe operation results as well, although in that case, the same tx on different forks would have to have a different object for each fork.

Limit delegate turnover rate to support SPV
-------------------------------------------

We should think about hard coding a limitation for the number of delegates that can change in a round.  I.e. if more than 25 new delegates in a round, only add the 25 most popular new delegates, and only remove the 25 least popular old delegates.  Limited rate of delegate turnover allows us to use SPV with DPOS.

Allow backreferences to object ID's in transaction
--------------------------------------------------

We need a way for a transaction to refer to objects it has newly allocated.   Let's reserve low object ID's for this purpose (perhaps 0-127, 0-255, or 0-1023).  So newly created objects start out with a low ID, remain there for the duration of the transaction, and are then moved to their permanent (high) ID.

It may shorten some transactions to also temporarily (within the transaction) assign low ID's to an object that is referenced in a transaction.  Thus, a transaction doing multiple actions on object #2,941,832 would only have the 4-byte varint encoding once; subsequent references to 2,941,832 within that transaction would instead call it object #5 (if objects 0-4 were already taken by other objects referenced by the same transaction).

Voting
------

Holders of any asset should be allowed to vote for object ID's representing *candidates*.  A delegate is a candidate, but we can define other candidate types as well.  All we need in the candidate structure is a `map<asset_id, vote_count>`.  If we're willing to restrict each candidate object to only being allowed to run in elections for a single asset, we'd effectively be restricting the `map` to contain at most a single entry, and thus could simply replace it with an `asset_id` and `vote_count`.

By removing restrictions on the voting system and leveraging the general architecture of the object graph, it will now be simple to implement voting on proposals.

In addition, as well as single-sig and multi-sig, we can now allow a new signature type for "the delegate(s) appointed by the owners of X", where X is any asset.  This allows systems for distributed escrow and ultimately building of new DPOS chains with their own delegates as apps on our platform.

Thoughts on prices
------------------

*Prices need overhauled*.  Having a `price` be decimal in BitShares is a pain point.  If you do that, you're asking for rounding errors -- and we sure have plenty.  Instead, `price` should be a rational number.

IMHO (M = @theoreticalbts), an offer should specify `have` and `want` instead of `quote` and `base`.  Then it stands for the half-open interval `(want / have, infinity)`.  I.e. if Alice has 10000 BTS and wants $90, then Alice will accept an exchange rate of $0.009 / BTS or more.  If Bob has $100 and wants 10000 BTS, Bob will accept an exchange rate of 100 BTS per dollar or more (i.e. $0.01 / BTS or less).  Two offers can be matched against each other if `alice.have.asset_id == bob.want.asset_id` and `alice.want.asset_id == bob.have.asset_id` (i.e. they are on opposite sides of the same market), and `alice.want.amount * bob.want.amount >= alice.have.amount * bob.have.amount`.  So in our example, the LHS "want product" is `$90 * 10000 BTS = 900,000 BTS` and the RHS "have product" is `$100 * 10000 BTS = 1,000,000 BTS`, so the parties can make a deal.  (The difference between the two is negative if there's a spread, positive if there's an overlap, zero if there's an exact match.)

You can create an order list for matching by comparing `want / have` and sending the smallest list to the top.  I.e. if Charlie has 10000 BTS and wants $95, comparing his offer to Alice's above would require comparing `charlie.want / charlie.have` to `alice.want / alice.have`.  The current market engine uses decimal fraction data type, however an exact comparison is also possible:  `charlie.want / charlie.have >= alice.want / alice.have` iff `charlie.want * alice.have >= alice.want * charlie.have`.

So you can compare orders on the same side of the book by comparing cross products, and check whether orders on opposite sides of the book overlap by comparing the "want product" to the "have product."

Undoing indexes
---------------

This looks like a can of worms:

```cpp
    struct undo_state
    {
        object_id_type                         old_next_object_id;
        map<object_id_type, packed_object>     old_values;
        map<string,object_id_type >            old_account_index;
        map<string,object_id_type >            old_symbol_index;
    };
```

The issue here is that you have to CoW the index entries of any indexed object.  This is done by `database::index_account` and `database::index_symbol` right now.  Which seems like a lot of boilerplate for adding an additional index.  Is there some way we can reduce the number of places we have to modify the code when adding a new index?

Edges have no purpose
---------------------

I don't see what functionality we gain by having an edge class.  Anything the edge can do, can be done by data object.

The one thing the edge provides is an index.  Having app-level transactions automatically indexed by the platform may ease implementation of some apps.  However we can get more general indexing by just letting us have two strings, then they can be indexed by any data (if the indexed field doesn't have to be an ID, it's a little more flexible).  You can use strings as ID's by encoding the ID with big-endian variable length encoding.  The existence and meaning of the account would already be asserted by the publishing transaction's reference block.

Should tx and block have object ID?
-----------------------------------

Maybe transactions and blocks should have their own ID's.

Types
-----

This is just some notes on types.  (This document started out as a spec.)

- `address` : An `address` is computed as `ripemd160( sha512( compressed_ecc_public_key ) )`.
- `asset` : An `asset` is a `share_type` specifying the amount, and an `asset_id_type` specifying an asset object ID.
- `price` : A `price` specifies an exchange rate being offered.  See "thoughts on prices" below for how prices should be overhauled.

