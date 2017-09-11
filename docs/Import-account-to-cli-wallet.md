Lets face it, in this days most of the bitshares accounts are created through OpenLedger.

As developers we like to control our accounts throw the command line. This small tutorial will help you import your light/web wallet created account into the cli wallet.

Everything is done with your private key, it is extremely important that you keep this key safe or as you will see anybody will be able to claim your funds.

Go to the permissions tab of your account:
![](http://oxarbitrage.com/bs/cli1.png)

Click in your public key and press "show" to get your private key.

If your wallet is locked you will need to enter your password to unlock and unhide the private key.

![](http://oxarbitrage.com/bs/cli2.png)

Thanks @valentin for showing me where how to obtain the private keys from the web wallet.

Now, the only thing you need to import to the cli wallet is your account name and your private key.

Start the cli wallet pointing it to a live node:

```
root@NC-PH-1346-07:~/bitshares/issue163/bitshares-core# ./programs/cli_wallet/cli_wallet --server-rpc-endpoint ws://localhost:8090
```

Set a password for your wallet, please note this password does not need to be the same as the one you have for OpenLedger, this is a new wallet and it will be secured by a new password, then we import the accounts into the new created wallet.

```
new >>> set_password mypass
set_password mypass
null
locked >>> unlock mypass
unlock mypass
null
unlocked >>> 
```

Using the private key use the following command to import account:

```
import_key "oxarbitrage.a699" privatekeygoeshere
```

And you are done, no need to claim balance, it is already there.

Use `list_my_accounts` to see your imported account.

And to check balance:

```
unlocked >>> list_account_balances oxarbitrage.a699
list_account_balances oxarbitrage.a699
31016.69330 BTS
0 CNY
0 USD

unlocked >>> 
```

You now control your web wallet created account from your cli wallet.
