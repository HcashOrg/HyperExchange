It is recommended that the reader has went through the following introductions:

* [[Introduction to Blockchain Objects|Blockchain-Objects]]
* [[Introduction to Wallet/Node daemons|Wallet_Full-Nodes_Witness_Nodes]]
* [[Graphene API|API]]
* [[Websocket Subscriptions|Websocket Subscriptions]]


To monitor accounts, we recommend to use the `get_full_accounts` call in order to fetch
the current state of an account and *automatically* subscribe to future account
updates including balance update.

A notification after a transaction would take the form:

    [[
        {
          "owner": "1.2.3184", 
          "balance": 1699918247, 
          "id": "2.5.3", 
          "asset_type": "1.3.0"
        }, 
        {
          "most_recent_op": "2.9.74", 
          "pending_vested_fees": 6269529, 
          "total_core_in_orders": 0, 
          "pending_fees": 0, 
          "owner": "1.2.3184", 
          "id": "2.6.3184", 
          "lifetime_fees_paid": 50156232
        }
    ]]

Please distinguish transactions from operations: Since a single transaction may contain several (independent) operations, monitoring an account may only require to investigate *operations* that change the account.