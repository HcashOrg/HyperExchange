# Wallet Backup Implementation

The purpose of this document is to outline the requirements for automated backup to prevent the loss of funds.  A web-based wallet is easily accidentally deleted anytime the user clears all cache and cookies and is not a safe place to keep unrecoverable information.  For this reason the wallet data must be backed up in a secure manner. 

## Tracking Backup Status 

The minimal requirement for wallet backups is for the user to download an encrypted copy of their wallet to their disk outside the browser cache / data stores.   Any time new, unrecoverable data is added to the wallet the user interface needs to indicate "Backup Recommended" on the footer of every screen.  When the user clicks "backup" it should automatically download the wallet to their Download's folder with no other action require.  Ideally, the user would not have to click anything to trigger the automatic saving to the Download's folder. 

## Automatic Uploading to Drop Box

This process can be automated as an alternative (or in addition to) downloading the wallet locally.  

## Encrypting the Wallet

Backups are only as secure as the password protecting the encrypted data.  Typical user passwords are less than ideal for the purpose of securing a backup, so instead it is preferred to encrypt backups relative to a brain key.  A brainkey is something users are expected to write down and physically store and then keep off of digital devices that may be lost.  

The brainkey can be used to derive a public key.  When a wallet is encrypted a one-time private key is generated and combined with the public brainkey to generate a onetime secret that will serve as the AES password.    

Users will be able to recover any backup with their brain key, but will not require their private brain key to encrypt their backup.

## Accessing the Wallet on New Device

A secondary purpose for remote backups is to allow the user to easily load their wallet on a new device by simply entering the brain key.   Using 3rd party services such as Drop Box or Github to track backups complicates the process of loading a wallet on a new computer.  Users will have a better experience if all backups are seamless and do not require signing or authorizing a 3rd party service. 

Users should be able to upload their encrypted wallet to the wallet server and then look it up again knowing nothing but the brain key.   The server will need a secure, automated way to authenticate the individual requesting to upload or download the wallet without requiring external verification. 

To achieve this wallets storage and retrieval will require authentication of the WALLET_KEY which is derived from the brain key.   The private WALLET_KEY is kept on the user's device protected only by their simple password.   Anytime the user's local wallet is unlocked it is possible to upload a backup.

When a user logs into the web service from a new device, they must provide their BRAIN_KEY which will allow them to generate their WALLET_KEY and therefore authenticate a request to download the wallet from the server.   The server will lookup the wallet using the PUBLIC_WALLET_KEY as the unique identifier. 

## Avoid Deleting Data

When a user does a backup to the server, old versions of the wallet should be saved.  This will prevent attackers who compromise a local wallet from overwriting a good backup with a bad backup.  

## Wallet Data Should be Compressed

To save space on the server, wallet data should be compressed prior to being encrypted.  

## Preventing Abuse 

Any service that allows users to upload arbitrary binary data has the potential to be abused to store data other than wallet backups.  There are several ways of preventing abuse:

1. Require all wallet keys to belong to an account registered on the blockchain
2. Limit automatic backup to accounts referred by a set of accounts
3. Limit the frequency of updates to the backup wallet 
4. Limit the frequency of downloads of the backup wallet 
5. Limit automatic backup to whitelisted accounts 
6. Limit automatic backup to accounts with a minimal balance 
7. Require proof-of-work on every backup

## Limiting Liability 

Account backup should be provided for convenience only, and should not be relied upon as a way to recover your wallet file in all circumstances.  The service provider should have all users waive the right to sue if the provider is unable or unwilling to produce a copy of the backup wallet.  Users should also agree to allow the service provider to publish all backup wallets in a public manner, such as a github repository, and therefore the service provider has no liability for data breaches.  










