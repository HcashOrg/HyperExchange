## Wallet Login Protocol

The idea behind the login protocol is to allow another party to verify that you are are the owner of a particular account.  Traditionally login is performed via a password that sent to the server, but this method is subject to [Phishing Attacks](https://en.wikipedia.org/wiki/Phishing).  Instead of a password, Graphene uses a cryptographic challenge/response to verify that a user controls a particular account.

For the purpose of this document, we will assume https://merchant.org is the service that will be logged into and that https://wallet.org is the wallet provider that will be assisting the user with their login.

## Step 1 - Merchant Login Button

The merchant must provide the user with a login button that links to https://wallet.org/login#${args} where ${args} is a JSON object serialized with the [Wallet Argument Format](Wallet Argument Format) and containing following information: 

```
{
   "onetimekey" : "${SERVER_PUBLIC_KEY}",
   "account"  : "${OPT_ACCOUNT_NAME}",
   "callback" : "https://merchant.org/login_callback"
}
```

The merchant server will need to save the `${SERVER_PRIVATE_KEY}` associated with the `${SERVER_PUBLIC_KEY}` in the user's web session in order to verify the login.  

## Step 2 - Wallet Confirmation

When the user loads `https://wallet.org/login#${args}` they will be prompted to confirm the login request by selecting an account that they wish to login with.  If "account" was specified in the `${args}` then it will default to that account.

After the account is identified enough keys to authorize a account must participate in the login process in the following way.

The wallet generates a `WALLET_ONETIMEKEY` and derives a `shared secret` with the `SERVER_PUBLIC_KEY` provided by the https://merchant.org via `${args}`.   This shared secret is a provably "random" 512 bits of data that is only known to the wallet at this point in time.   The wallet then gathers signatures on the shared secret from enough keys to authorize the account.  In the simple case this will be a single signature, but in more complex cases multi-factor authentication may be required.  

After gathering all of the signatures the wallet redirects the user to `https://merchant.org/login_callback?a=${result}` where `result` is an encoded JSON object containing the following information:

```
{
   "account": "Graphene Account Name",
   "server_key": "${SERVER_PUBLIC_KEY}",
   "account_key": "${WALLET_ONETIMEKEY}",
   "signatures" : [ "SIG1", "SIG2", .. ]
}
```

## Step 3 - Server Verifies Authority 

Upon receiving the `result` from the wallet, https://merchant.org will lookup ${SERVER_PRIVATE_KEY} in the user's session data and then combine it with `${WALLET_ONETIMEKEY}` to generate the `shared secret` that was used by the wallet.   Once this shared secret has been recovered, it can be used to recover the public keys that correspond to the provided signatures. 

The last step is to verify that the public keys provided by the signatures are sufficient to authorize the account given the current state of the graphene blockchain.   This can be achieved using the witness API call `verify_account_authority( account_name_or_id, [public_keys...] )`.   The `verify_account_authority` call will return `true` if the provided keys have sufficient authority to authorize the account, otherwise it will return `false`

 



