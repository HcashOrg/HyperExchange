To maintain a user experience similar to any other website, it is critical that users can log into the website and gain access to their private keys from any computer.  It is equally important that the server does not have access to the user's private keys.

The goal of this specification is to define a protocol between the web browser and the server for storing and retrieving private data of the user.  Because all data is encrypted, the server will need to use some techniques to prevent abuse.  We will assume that prior to storing data on the server the user must request a code that will be sent to them by email.

The syntax below is shorthand for JSON-RPC requests sent to/from the server:

    server.requestCode( email )

Then the user can store data on the server like so:

    server.createWallet( code, encrypted_data, signature ) 

The wallet will be saved on the server using the *public_key* derived from *encrypted_data* and *signature*.  The code contains a irreversible hash of the email.  This is used in a unique constraint effectively limiting users to one wallet per email.  The original email address is not stored in the database.

Then the user can fetch data from the server like so:

    server.fetchWallet( publickey, local_hash )

This method will only fetch the wallet if it is different than what is already cached locally.  In addition to returning the encrypted wallet, this method will also return statistics about when it was last updated, and which IPs have requested the wallet and when.  This can be used by the user to detect potential attempts at fraud.

The user can update their data with this call:
   
    server.saveWallet( original_local_hash, encrypted_data, signature )

The signature can be used to derive the public key under which encrypted_data should be stored.  The server will have to verify that derived public key exists in the database or this method will fail.  If the original_local_hash does not match the wallet being overwritten this method needs to fail.

The user can change their "key" with the following call:

    server.changePassword( original_local_hash, original_signature, new_encrypted_data, new_signature )

After this call the public key used to lookup this wallet will be the one derived from *new_signature* and *new_encrypted_data*.  A wallet must exist at the *old_public_key* derived from the *original_local_hash* and *original_signature*.


The user may delete their wallet with the following call:

    server.deleteWallet( local_hash, signature )

For security reasons, this delete is permanent.  The user may repeat the process to create a new wallet using the same email.

## Generating Public Keys

Users generate a private key from their password.  Generally speaking, passwords are too weak and could be strengthened by being combined with other information that is both unique to the user and unlikely for the user to forget.  At a minimum email address and/or username can be combined with the password to recover the private key for the wallet.  Additional information such as social security numbers, passport IDs, phone numbers, or answers to other security questions may be useful for some applications.  Keep in mind that the user must remember both their password AND the exact set of security Questions **and** Answers in order to recover their account.

## Mitigating Brute Force Attacks

The server should limit the number of wallet requests it accepts per IP address to a fixed number per hour.  In this way users wallet data is kept secure from attempts at brute force attacks unless the server itself is compromised.  In the event the server is compromised, users are only protected by the quality of their password and any other data used as salt.

## Saving the Wallet Locally 

The browser should cache the wallet data locally 


