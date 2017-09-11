## Stealth Transfers

This tutorial shows how to use the CLI wallet to perform stealth transfers in BitShares.   A stealth transfer is one that hides both the amount being sent and the parties involved in the trade.   Stealth transfers are also referred to as blinded transfers.  When privacy is important no account is ever used twice and it is impossible for any third party to identify how money is moving through blockchain analysis alone.

### Creating a Blind Account

Blind Accounts are not registered on the blockchain like the named accounts on BitShares.  Instead a blind account is nothing more than a labeled public key.  The label assigned to the key is only known to your wallet.  The first step is to create an blind account by giving the wallet a name and a "brainkey".  A "brainkey" is effectively the private key used by your account. 

    >>> create_blind_account alice "alice-brain-key"
        "BTS7vbxtK1WaZqXsiCHPcjVFBewVj8HFRd5Z5XZDpN6Pvb2dZcMqK"

The result of this call is to print out the Public Key associated with blind account *alice*.  

For the purpose of this test we will create two blind accounts in two different wallets (alice and bob).

    >>> create_blind_account bob "bob-brain-key"
        "BTS8HosMbp7tL614bFgqtXXownHykqASxwmnH9NrhAnvtTuVWRf1X"  

Alice and Bob now each have their own account in their own wallet that isn't known to anyone else in the world.  They can view their blind accounts with the following command:

    >>> get_my_blind_accounts
    [[
    "alice",
    "BTS7vbxtK1WaZqXsiCHPcjVFBewVj8HFRd5Z5XZDpN6Pvb2dZcMqK"
    ]]

### Adding a Contact

Suppose Alice wishes to make a payment to Bob, she must first ask Bob for his account public key, BTS8HosMbp7tL614bFgqtXXownHykqASxwmnH9NrhAnvtTuVWRf1X.   Then she can assign a label to it in her wallet.  In this case she will record bob's key as "bobby".

    >>> set_key_label BTS8HosMbp7tL614bFgqtXXownHykqASxwmnH9NrhAnvtTuVWRf1X bobby

   
### Transferring to a Stealth Balance 

All balances must come from somewhere and initially all balances are held by some public account.  For the purpose of this example we will assume alice has an account with the name 'alicepub' has a public balance that it would like to obscure.   It will do this by transferring some funds to one or more blind accounts.

    >>> transfer_to_blind alicepub BTS [[alice,1000],[alice,2000]] true
    alicepub sent 3000 BTS to 2 blinded balances fee: 15 BTS
    1000 BTS to  alice
	  receipt: 2Dif6AK9AqYGDLDLYcpcwBmzA36dZRmuXXJR8tTQsXg32nBGs6AetDT2E4u4GSVbMKEiTi54sqYu1Bc23cPvzSAyPGEJTLkVpihaot4e1FUDnNPz41uFfu2G6rug1hcRf2Qp5kkRm4ucsAi4Fzb2M3MSfw4r56ucztRisk9JJjLdqFjUPuiAiTdM99JdfKZy8WTkKF2npd

    2000 BTS to  alice
	  receipt: 28HrgG1nzkGEDNnL1eZmNvN9JmTVQp7X88nf7rfayjM7sACY8yA7FjV1cW5QXHi1sqv1ywCqfnGiNBqDQWMwpcGB1KdRwDcJPaTMZ5gZpw7Vw4BhdnVeZHY88GV5n8j3uGmZuGBEq18zgHDCFiLJ6WAYvs5PiFvjaNjwQmvBXaC6CqAJWJKXeKCCgmoVJ3CQCw2ErocfVH

In this case the only thing the public sees is that account 'alicepub' sent 3000 BTS to two different places.  The outside world has no idea how much is in each output, only that they add up to 3000 BTS.

### Viewing Blind Balances

Alice can verify that she has 3000 BTS in blinded balances by using the command:

    >>> get_blind_balances alice
    3000 BTS

### Stealth Transfer to Bob

Alice can now transfer to "Bob" which she has labeled 'bobby' in her wallet via the following command:

    >>> blind_transfer alice bobby 500 BTS true
    blind_transfer_operation temp-account fee: 15 BTS
    485 BTS to  alice
	  receipt: iLrPEY61BQsrKSVLLhuJBB6axkjpp2YA1EUq8k8tdQNfbgm1rZn8iUfxd2szyLV1962S39VtPFcuidok7tnT851JFUvP5r7U5MfbtRvmsNBHtSmaWyfbXg7srPsp1roUBpr9Z2QM7W7X5AAonFqoduWcnGp7cViQCDppEqSZHGjY8zFJARd1vm4qoPcMAjw4pjS3vgj6796SfR9ntnN5vZr5b9WvM4Hune7DfbGShed81n1R63BH9h9Ef8BXRy1ERkkJhMmYhXKC

    500 BTS to  bobby
	  receipt: iLrPEY61BQsrKSVLLhuJBB6axkjpp2YA1EUq8k8tdQNfbgmWNQD9tWnAciMpPuLhanv4j8nhvUE1ZjD3WNZPoxdiekTCraMir7xx5rbZsGCogF6YfPbCnZCapMDkC8Zsgs5bZWCB2oRvB1wCjYmsQaji6SQcax5Sii4MY93Q1HGPvehcS7jBvLDz5e1GQmAzoWhnPZqoCuDSvL521CSCCxRvLXoHK1Rih5kX72tJYdAXCECUL3xZ2cd2CA8eegfTiC7f7XkTd75f

The output shows that 500 BTS was sent to bobby and 485 BTS sent back to alice as change after paying a 15 BTS fee.  If we check the balance of alice we will see: 

    >>> get_blind_balances alice
    2485 BTS

The outside world only knows that alice sent some amount less than 3000 BTS to two new outputs.

### Receiving a Blind Transfer

At this point Bob has not actually received any funds because his wallet has no idea where to look.  He needs to load the receipt from Alice into his account.

    >>> receive_blind_transfer iLrPEY61BQsrKSVLLhuJBB6axkjpp2YA1EUq8k8tdQNfbgmWNQD9tWnAciMpPuLhanv4j8nhvUE1ZjD3WNZPoxdiekTCraMir7xx5rbZsGCogF6YfPbCnZCapMDkC8Zsgs5bZWCB2oRvB1wCjYmsQaji6SQcax5Sii4MY93Q1HGPvehcS7jBvLDz5e1GQmAzoWhnPZqoCuDSvL521CSCCxRvLXoHK1Rih5kX72tJYdAXCECUL3xZ2cd2CA8eegfTiC7f7XkTd75f "alice" "memo"
    500 BTS  alice  =>  bob   memo

    >>> get_blind_balances bob
    500 BTS

The call to receive a blind transfer takes two optional arguments, "from" and "memo" which will be used to label alice's public key in bob's wallet.  This helps bob to make sense of his transfer history.

    >>> blind_history bob 

    WHEN           AMOUNT  FROM  =>  TO  MEMO
    ====================================================================================
    19 seconds ago  500 BTS  alice  =>  bob  memo

### Transferring back to Public 

Eventually every blind balance needs to convert back to a public balance which can be achieved with the following command:

    >>> transfer_from_blind alice alicepub 1000 BTS true 
    { ... }

In this case alice returned some of her remaining blind balances back to her public balance.
