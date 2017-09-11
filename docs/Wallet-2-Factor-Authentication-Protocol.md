## Two Factor Authentication Protocol

Two factor authentication is critical for maximizing security and ease of use.  In the case of cryptocurrencies two-factor authentication means gathering multiple signatures on a transaction to authorize a transfer.  The process of gathering these signatures requires passing around a transaction digest to multiple parties who will automatically provide their signature after verifying your identity by some means. 

The purpose of this protocol is to define a standard way for 3rd parties to provide automatic 2-factor authentication services with any standard Graphene wallet.  Users of a Graphene wallet can easily add any number of 2-factor authentication (2FA) providers to their account and the Graphene wallet will use this protocol to gather the required signatures for the transaction.

### User Accounts

A user wishing to add one or more 2FA providers to their account will need to update their account permissions to require the authority of one key from the user *AND* the authority of a second account controlled by the 2FA provider.   The purpose of this protocol is to help automate the gathering of any additional signatures in a standardized manner. 

### 2FA Providers 

For the purpose of this document `https://secondfactor.org` will be the example service provider and `alice` will be the account name of the user seeking services from secondfactor.org.

## Step 1 - User Registration

The first thing a user must do is identify themselves with `secondfactor.org` by some means.  This may be as simple as verifying an email address, registering a phone number, or ordering a keyfob. 

## Step 2 - Create Graphene Account for secondfactor.org

A Graphene account is how `secondfactor.org` authenticates itself with a Graphene blockchain. We will assume `secondfactor.org` has registered the Graphene account name `sfactor`.  

## Step 3 - Create a Dedicated Graphene Account for the alice/sfactor Combination

When `alice` approaches `sfactor` and signs up for 2FA she needs to add a second signer on the account `alice`.   The goal is for `sfactor` to sign anything at the request of `alice` but to do this without compromising the `sfactor`.  To do this a new account must be registered on the blockchain that is effectively controlled by `alice` via `sfactor`.  We will call this new account `sfactor.alice`.   `sfactor.alice` will be configured so that it's owner authority is `sfactor`, but it's active authority is a unique public key assigned to the account by `sfactor`.    At any time `sfactor` may update the active authority of `sfactor.alice` using the owner permissions.   

## Step 4 - Add sfactor.alice as a required authority for alice

Alice will have to update the active permissions of account `alice` to require both her key and the permission of `alice.sfactor` to approve any action on the part of `alice`.    She can either do this manually *or* https://secondfactor.org can use the [Transaction Request Protocol](Transaction Request Protocol) to automatically ask `alice` to approve the change to her account.

## Step 5 - Alice Requests an Action requiring 2FA 

After the `alice` account has been updated it will now require permission from `alice.sfactor` to authorize any action.
The most basic 2-factor API assumes the provider.



reates an account with `secondfactor.org`

