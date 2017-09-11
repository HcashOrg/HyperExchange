
## How to Import Large Wallets from BitShares 0.9.3

In bitshares 0.9.3c, run: 
    
    wallet_export_keys /tmp/final_bitshares_keys.json

in bitshares 2.0 CLI wallet, run:
    >>> import_accounts /tmp/final_bitshares_keys.json my_password

then, for each account in your wallet (run list_my_accounts to see them):

     >>> import_account_keys /tmp/final_bitshares_keys.json my_password my_account_name my_account_name

note: in the release tag, this will create a full backup of the wallet after every key it imports.
If you have thousands of keys, this is quite slow and also takes up a lot of disk space.
Monitor your free disk space during the import and, if necessary, periodically erase the
backups to avoid filling your disk.  The latest code only saves your wallet after all keys have been imported.  

     >>> import_balance my_account_name ["*"] true
     >>> list_account_balances my_account_name

Verify the Results