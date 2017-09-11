Plasma Wallet API
-----------------

This API defines how developers can create and manage a Wallet using Plasma. A Wallet is a place where private user information can be stored.  This information is kept encrypted when on disk or stored on the remote server.

The purpose of this API is to abstract the details of synchronizing local and remote copies of your wallet.

Assume a class named **Wallet** that has the following calls:

This method will configure the wallet to look to a remote host to load and/or save your wallet. By passing *null* into this call the wallet will stop synchronizing its state with the remote server.
```
wallet.useBackupServer( url )
```

This call is used to configure the wallet to keep a local copy on disk. This allows the user to access the wallet even if the server is no longer available. This option can be disabled on public computers where the wallet data should never touch disk and should be deleted when the user logs out. 
```
wallet.keepLocalCopy( save = true )
```

This method is used to configure the wallet to save its data on the remote server. If this is set to false, then it will be removed from the server. If it is set to true, then it will be uploaded to the server. If the wallet is not currently saved on the server a token will be required to allow the creation of a new wallet.
```
wallet.keepRemoteCopy( save = true, token = null )
```

This API call is used to load the wallet. If a backup server has been specified then it will attempt to fetch the latest version from the server, otherwise it will load the local wallet into memory.  The configuration set by *keepLocalCopy* will determine whether or not the wallet is saved to disk as a side effect of logging in.  The wallet is unlocked in RAM when it combines these as follows: lowercase(email) + lowercase(username) + password to come up with a matching public / private key.  If *keepRemoteCopy* is enabled, the email used to obtain the token must match the email used here.  Also, if *keepRemoteCopy* is enabled, the server will store only a one-way hash of the email (and not the email itself) so that it can track resources by unique emails.

```
wallet.login( email, username, password )
```

This API call will remove the wallet state from memory.
```
wallet.logout()
```

This method returns a JSON object representing the state of the wallet. It is only valid if the wallet has successfully logged in.
```
wallet.getState()
```

This method is used to update the wallet state. If the wallet is configured to keep synchronized with the remote wallet then the server will refer to a copy of the wallets revision history to ensure that no version is  overwritten.  If the local wallet ever falls on a fork any attempt to upload that wallet will cause the API call to fail. The user should be alerted so they can reconcile manually.  After successfully storing the state on the server, save the state to local memory, and optionally disk.  
```
wallet.setState( state ) 
```
