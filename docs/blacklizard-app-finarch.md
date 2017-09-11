
Financial architecture of distributed applications
--------------------------------------------------

This paper summarizes the organizational structure of Black Lizard, with a focus on how other DAC's ("apps") will use base services provided by the Black Lizard blockchain to enable unambiguous consensus (Byzantine generals problem), flexible ownership and monetization structures (completely centralized, completely decentralized, or many points in between), and effective operational integration with BitAssets.

This paper is an incomplete draft, more of a starting point for discussion than a final product.

Black Lizard:  Be the platform, not the app
-------------------------------------------

The new business direction will point us directly to competing with Ethereum as a platform for smart organizations and smart contracts.

Specifically, the architecture of the Ethereum blockchain as executing Turing-complete contracts is a weak point from a standpoint of scalability and flexibility:

- Scalability:  Patricia tries are (Dan claims) slow.
- Scalability:  Every node must execute every contract.
- Flexibility:  Giving resources to a particular piece of code means that code cannot be upgraded or bugfixed; the new version will be a *different* piece of code with no access to the old version's resources.

Project Black Lizard involves a substantial rewrite of the BitShares blockchain database and transactions.

From the business side, Project Black Lizard has two goals:

- Provide a platform layer with sufficient flexibility to enable many client applications.
- Use the platform to develop app(s) to provide Vote functionality.

What is an app?
---------------

A Black Lizard *app* is any DAC, other than Black Lizard itself, which is hosted on the Black Lizard blockchain.

Entities on the Black Lizard blockchain that do not belong to any app are *platform level* entities.  For example, the *core asset* (Black Lizard equivalent of BTS), BitAssets and UIA's unassociated with an app are considered *platform assets*.

What are the hard problems?
---------------------------

- Hard Problem 1:  Platform asset interaction problem.  Apps must interact with platform assets.  Thus, the platform must support a notion of "platform assets controlled by an app".
- Hard Problem 2:  State import problem.  Transactions may involve app-specific semantics (including arbitrary off-chain information).  Enough information about these arbitrary semantics must be "imported" to the blockchain to at least allow proper distribution of platform assets.
- Hard Problem 3:  Proof-of-broadcast problem.  The platform can provide timestamping of hashes of arbitrary data.  However, this is not sufficient.  Such a hash is meaningless without proof that some quorum of the app's validators have seen (and presumably archived) the data in question.  This is effectively providing a resource-efficient and sybil-resistant proof-of-broadcast.
- Hard Problem 4:  Dead validator problem.  Any mechanism which solves Hard Problem 3 must be resilient against dead validators.
- Hard Problem 5:  Monetization problem.  In order to incentivize development of apps, app developers need to have a mechanism to get paid.

Solution overview
-----------------

The solution to the hard problems is to give apps a similar DPOS structure to the platform itself.  Each app should have an equity asset representing ownership, which votes for up to 101 trusted validators ("app delegates").  After transactions are published on the platform chain, app validators can ratify or veto them.  A transaction involving conditional transfer of platform assets dependent on app-specific off-chain conditions, requires a majority of app delegates to ratify what should actually happen before platform assets are transferred.

This solves Hard Problem 1 (app's platform assets are controlled by a majority of app delegates), Hard Problem 2 (app delegates are responsible for importing all off-chain app state), Hard Problem 3 (app delegates don't ratify transactions that imply off-chain broadcasts unless a majority have seen and stored the data), Hard Problem 4 (dead app delegates can be replaced by equity asset holders' votes), and Hard Problem 5 (app's platform assets can be transferred to equity asset holders to "pay a dividend").

Oracle invocation
-----------------

App delegates can have three actions:  Ratify, veto, or fail to reach consensus.  The general behavior for a transaction is to specify an operation for each of these paths.  I.e. a transaction needs to be able to say:

    switch( state )
    {
    case ratified:
        op_pay_app_fee_to_implement_transaction;
        op_transfer_resources_to_app;
        op_transfer_resources_from_app;
        break;

    case vetoed:
        op_pay_app_validation_fee;
        break;

    case fail_to_reach_consensus:
        /* do nothing */
        break;
    }

It is clear then that such a transaction should be published, but its execution deferred until the app delegates have reached consensus (or a specified expiration time has been reached without consensus), in which case one of several paths are taken.

Since Black Lizard ops maintain all chain invariants, I think we should allow `switch` in the *middle* of a transaction, as well as the beginning.  This will enable a decentralized altcoin bridge algorithm:

    op_lock_asset <alice> <m> <bitbtc>
    state = op_invoke_oracle <bridge_app_id> <return 0 if <btc_txid> exists and sends <n> BTC from <btc_bob_addr> to <btc_alice_addr>, return 1 if <btc_txid> exists but doesn't do that, or any other transaction spends <btc_bob_addr> >
    switch( state )
    {
    case 0:
        /* bob sent bitcoins to alice */
        op_pay_app_fee;
        op_send_locked_asset <bob> <m> <bitbtc>
        break;

    case 1:
        /* bob spent bitcoins elsewhere */
        op_pay_app_fee;
        op_send_locked_asset <alice> <m> <bitbtc>
        break;
    }

This is basically an escrow system.  We can add an expiration time and a `fail_to_reach_consensus` case, however this doesn't quite work -- Alice can end up with both BTC and BitBTC if the Bitcoin network is too slow confirming Bob's transaction!  Instead, before Alice publishes the above Black Lizard transaction, she should require Bob to send her a *cancellation transaction* locked in the future (while Bitcoin doesn't have the ability to expire transactions, it *does* have a field called `nLockTime` which makes a transaction invalid until some point in the future).  The cancellation transaction simply sends coins from `btc_bob_addr` to some other address `btc_bob_addr2` presumably controlled by Bob.  If Alice doesn't receive the BTC in time, she is able to broadcast the cancellation transaction, which triggers case 1 and unlocks Alice's BitBTC while returning the BTC to Bob.  (If the cancellation transaction travels over a broadcast medium, the transaction should be encrypted to Alice's public key.  This way, Alice has the option to give Bob an extension of time; she can simply wait to publish the cancellation transaction.  If the cancellation transaction was broadcast to e.g. the whole Black Lizard network unencrypted, arbitrary third parties can take away Alice's option by publishing the cancellation transaction to the Bitcoin network themselves.)

N.b. the `op_invoke_oracle` is instructions to the *bridge app*, the platform does not need to know how to interpret it.  All the platform "knows" is that the oracle now has the ability to create a transaction saying either "the result of the `op_invoke_asset` in block <height>, transaction <tx_index>, operation <op_index> is 0" or "the result is 1", which causes the platform to run the respective case.  The locked balance goes into a special account-like object belonging to this transaction, which is destroyed when the transaction finishes.  We should validate that if control reaches the end of the transaction, no locked assets remain (i.e. every control path results in spending all locked assets).  This is actually possible since operations establishing or disposing of locked balances always use literal amounts (i.e. you can't send an amount determined by a computation).

Bond markets with oracles
-------------------------

Alice lending Bob platform assets:

    op_lock_asset <bob> <collateral>
    op_transfer <alice> <bob> <debt>
    state = op_invoke_oracle <bond_app_id> <return 0 if <bob> sends <debt_plus_fee> to <alice>>
    switch( state )
    {
    case 0:
        /* loan repaid on time */
        op_send_locked_asset <bob> <collateral>
        break;

    case fail_to_reach_consensus:
        /* loan expired */
        op_send_locked_asset <alice> <collateral>
    }

Oracle notes
------------

Before the first `op_invoke_oracle` operation, the transaction can do any operations.  After `op_invoke_oracle`, it can only do ops that are guaranteed to be valid at any time in the future, i.e. only totally prefunded operations like distributing locked assets are allowed.

Since `op_oracle_result` can cause 



App operational accounts
------------------------

Each app has an `app_record`.  The `app_record` is similar to a user account structure, it has a name and controls balances (although withdrawing balances works differently from normal accounts).

The `app_record` specifies an *equity asset* representing ownership in the DAC.  The equity asset is a normal UIA, it can be a decentralized asset with unrestricted transfer capabilities, or it can be centralized (the issuer can whitelist holders, it can have (relinquishable) owner permissions for freezing, revocation, issuing new shares, raising issuance cap, etc.)

The `app_record` also specifies 1-101 *app delegates*.  The app delegates are effectively a multisig authority over the asset balance.  If the `app_equity_can_vote_for_delegates` flag is set in the app record, app equity shares can vote for app delegates.  If `app_equity_round_reward` is set to a number greater than zero, app delegates receive that much equity the first time they sign an app state in a platform-level round.

A platform-level transaction is said to *invoke* an app if it contains an `invoke_app` operation.  The `invoke_app` operation has three fields:

    struct op_invoke_app
    {
        app_id app;
        pair<asset_id, amount> app_fee_immediate;
        pair<asset_id, amount> app_fee_on_accept;
    };

The 



An operational account has one or more *app delegates* which approve an *app state* using DPOS.

Financial architecture of distributed applications
--------------------------------------------------

When an app is created, it is given an *app ID*, and an account-like record.  The app ID controls the operational funds.

An app has *app delegates* and *app equity shares*.  App equity shares are a UIA which represent the owners of an app (receive income generated by running the app), while app delegates represent the management of an app (responsible for operations including timely transfers of funds according to app semantics).

If `app_equity_can_vote_for_delegates` flag is set in the app record, app equity shares can vote for app delegates.  If `app_equity_max_inflation` is set to a number greater than zero, app delegates can earn equity by signing off on app state.  The `app_num_delegates` field tells how many delegates the app has -- an app can have a single delegate.

App delegates collectively have authority over the semantics of the app.  In particular, platform assets (i.e. the core asset, BitAssets or UIA's) sent to the app ID are redistributed among users of the app according to the consensus of the app delegates.  Thus you can have an app for e.g. a prediction market which takes BitUSD from some users and redistributes to other users based on arbitrary things.

Any app delegate can initiate a transfer from the app's operational funds.  Such a transfer is included in the signed state as well.  This is the only way for funds to move out of an app's operational balance.

A *signed state* consists of some set of transactions which have been included in blocks and are considered to be valid by an app delegate.  In order to update their app state, an app delegate can create a `sign_app_state` operation indicating a block height and veto set.  A transaction is included in the signed set if it is in a block at or below the indicated height, includes at least one transfer from the app ID, and is *not* in the veto set.  (Since the TaPoS-like inclusion of a block hash in the signature generation means the transaction cannot be replayed on a fork where the block in question has a different hash, noting the block height suffices).

The *app state* is a signed state which a majority of app delegates have approved.  Transactions are no-ops except for fees and app fees until they are included in the app state.  If a majority of app delegates veto a transaction, it is never included in the app state and is always a no-op except for fees and app fees.

There is a separate `pay_app_fee` operation.  The `pay_app_fee` operation has similar semantics as a transfer to the app account. *except* it remains active even if the block has been vetoed by the app in question.  For apps where fully evaluating a transaction is very expensive, `pay_app_fee` allows app delegates to be assured the transaction publisher will reimburse them for the evaluation costs even if they veto the transaction (by definition, app delegates don't know a transaction won't be vetoed until they evaluate it).

In other words, once BitUSD or core shares are released from the app account's balance, it becomes fungible with all other BitUSD.  If a majority of app delegates later decide that the BitUSD shouldn't have been distributed after all, there's no way for the app to get the BitUSD back -- if the platform allowed them to do so, we could end up unwinding many unrelated transactions the BitUSD participated in, causing loss of funds to innocent third parties!  The only way to be sure the majority of app delegates will never decide in the future to veto it, is to wait until a majority have passed on their opportunity to veto.

Notes:

- Transfers of app equity shares and updates of app delegate votes *are not* signed off by app delegate(s), thus equity holders retain the ability to fire app delegates even in the case where an app has a single delegate!
- The app state is the minimum amount of app semantics the platform needs to know about in order to distribute platform asset denominated operational funds.
- The app may have additional semantics that depend on other things in the Black Lizard blockchain, or indeed any external blockchain, or for that matter the actual world.  However, the platform need not know about these additional semantics to evaluate whether the app delegates have reached a consensus on how to distribute platform assets.
- The signed set of a particular app delegate must be monotonically increasing.  Thus, an app delegate cannot include a transaction in a veto set if it was included in a previous signed set for that app delegate.
- Transactions involving multiple apps need to be thoroughly specced and tested.
- App delegates need not sign in any particular order; there is no "signing schedule."

Signing efficiency concerns
---------------------------

A `sign_app_state` operation can include signatures from any number of app delegates; a transaction can include any number of `sign_app_state` operations; any number of `sign_app_state` transactions can be included in a block (within network size limits).  As a consequence of the previous, including a transaction in the app state takes at least two blocks -- one block to include the transaction, a second block to include `sign_app_state` from the majority of delegates.  This is the fundamental limitation of the architecture.

App delegates are free to defer publishing signatures indefinitely.  An app which recommends its delegates to wait for longer periods is essentially deliberately choosing to have a longer block interval.  The benefit of doing so is reducing transaction size and platform fees.

It would be technically possible to allow a majority of delegates to sign particular txid's.  But that would open the door to app-level double spending attacks.  For most apps, the validity of an app transaction depends on the transaction history of that app.  So signing a particular txid would be something an app delegate acting in good faith could never do, because they can't know the tx won't become invalid between the time it's signed and the time it's included in a block.

Allowing to sign a txid *and app history* is a viable approach, but seems overly complicated and limited scalability.

Distribution of app income
--------------------------

App delegates should regularly send operational funds designated as profits to the app's equity UIA address.  The platform will then distribute these proportionally to the equity holders.  Profits may come from fees or other sources depending on the app's business model.

The question is how to efficiently distribute the equity -- which is effectively the same problem as distributing yield.  (I.e., yield could be viewed as the DAC sending BitUSD-denominated fees to the BitUSD asset's address for proportional distribution to BitUSD holders.)

The simplest way is to iterate through all equity asset balances and increment them appropriately.  The problem with this approach is it places all the resource load for evaluation in a single block.  The solution is *lazy evaluation*.  For each balance, as well as the balance amount and asset ID, we should track when the balance last changed.  If the balance has been affected by a dividend since it last moved, it is marked as *dirty*.

Any operation which touches a dirty balance results in placing the dirty balance in a list of balances with pending dividends, then restarting with a clean balance of zero.  Thus, senders can still send people money even if the recipient has a pending dividend.  Withdrawing from a balance with a pending dividend will fail!

For this reason we have the `claim_dividend` and `disclaim_dividend` operations.  The `claim_dividend` will remove all balances of an asset from the list of balances with pending dividends; the claimed amount must be equal to the pending dividend.  The `claim_disclaim_dividend` operation has a claim amount which must be less than or equal to the pending dividend; and the sum of the claim and disclaim amount must be greater than or equal to the pending dividend.

The purpose of `claim_disclaim_dividend` is to allow withdrawal transactions to be created by cold storage solutions which may have multi-hour turnaround time.  I.e. the cold storage might publish a withdrawal transaction claiming a dividend of $0.03 / share which fails to validate because the actual dividend was $0.0301 / share (`claim_dividend` must be exact).  The `claim_disclaim_dividend` can allow a margin of error, the wallet can publish "I think the dividend is $0.03 / share, but I know this may be based on outdated information and the dividend may have increased.  So if additional dividends up to $0.0050 / share exist, proceed with the transaction anyway (I agree not to claim the excess)."


