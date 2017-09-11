
This page shows how the committee account can act using the proposed
transaction system.  Specifically, as an example I'm using the
creation of BitShares proposal 1.10.21, a proposal to update a
committee-controlled BitAsset to reduce `maximum_force_settlement_volume`
for asset `CNY` from 2000 (20%) to 200 (2%).

First check the asset to see what its current configuration is:

    >>> get_asset CNY
    {
      ...
      "bitasset_data_id": "2.4.13"
    }

Then check its bitasset object to get the currently active options:

    >>> get_object 2.4.13
    {
      ...
        "options": {
          "feed_lifetime_sec": 86400,
          "minimum_feeds": 7,
          "force_settlement_delay_sec": 86400,
          "force_settlement_offset_percent": 0,
          "maximum_force_settlement_volume": 2000,
          "short_backing_asset": "1.3.0",
          "extensions": []
        },
      ...
    }

Then do `update_bitasset` to update the options.  Note we copy-paste
other fields from above; there is no way to selectively update only one field.

    >>> update_bitasset "CNY" {"feed_lifetime_sec" : 86400, "minimum_feeds" : 7, "force_settlement_delay_sec" : 86400, "force_settlement_offset_percent" : 0, "maximum_force_settlement_volume" : 200, "short_backing_asset" : "1.3.0", "extensions" : []} false

If this was a privatized BitAsset (i.e. a user-issued asset with feed),
you could simply set the `broadcast` parameter of the above command to
`true` and be done.

However this is a committee-issued asset, so we have to use a proposed
transaction to update it.  To create the proposed transaction, we use
the transaction builder API.  Create a transaction builder
transaction with `begin_builder_transaction` command:

    >>> begin_builder_transaction

This returns a numeric handle used to refer to the transaction being built.
In the following commands you need to replace `$HANDLE` with the
number returned by `begin_builder_transaction` above.

    >>> add_operation_to_builder_transaction $HANDLE [12,{"fee": {"amount": 100000000, "asset_id": "1.3.0"}, "issuer": "1.2.0", "asset_to_update": "1.3.113", "new_options": { "feed_lifetime_sec": 86400, "minimum_feeds": 7, "force_settlement_delay_sec": 86400, "force_settlement_offset_percent": 0, "maximum_force_settlement_volume": 200, "short_backing_asset": "1.3.0", "extensions": []}, "extensions": []}]
    >>> propose_builder_transaction2 $HANDLE init0 "2015-12-04T14:55:00" 3600 false

The `propose_builder_transaction` command is broken and deprecated.  You
need to recompile with
[this patch](https://github.com/cryptonomex/graphene/commit/7a5c5c476d9762cbba1d745447191523ca5cd601)
in order to use the new `propose_builder_transaction2` command which allows you to set the proposing account.

Then set fees, sign and broadcast the transaction:

    >>> set_fees_on_builder_transaction $HANDLE BTS
    >>> sign_builder_transaction $HANDLE true

Notes:

- `propose_builder_transaction2` modifies builder transaction in place.  It is not idempotent, running it once will get you a proposal to execute the transaction, running it twice will cause you to get a proposal to propose the transaction!
- Remember to transfer enough to cover the fee to committee account and set review period to at least `committee_proposal_review_period`
- Much of this could be automated by a better wallet command.
