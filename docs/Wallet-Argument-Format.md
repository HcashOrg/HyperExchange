When using the [Wallet Login Protocol](Wallet Login Protocol) or the [Wallet Merchant Protocol](Wallet Merchant API) it is necessary to pass arguments to and from the wallet.   For security purposes these arguments are passed via the URL after the `#` so that they are kept secret from the wallet service provider.   Because URLs are limited to about 2048 bytes of data the Wallet APIs pass arguments as a JSON string that has been compressed with LZMA and serialized to Base58.

## Step 1: Compress your JSON representation

Using [LZMA-JS](https://github.com/nmrugg/LZMA-JS/) library to compress the JSON into a binary array.  This will be the most compact form of the data.  This will reduce the size of the data by up to 50%.

## Step 2: Convert to Base58 

Using the [bs58](http://cryptocoinjs.com/modules/misc/bs58/) library encode the compressed data in base58.  Base58 is URL friendly and size efficient.  After converting to base58 the result will be about 70% the size of the original JSON data.

