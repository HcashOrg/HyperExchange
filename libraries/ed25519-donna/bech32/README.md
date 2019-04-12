# The bech32(1) Utility

## Encode and decode [Bech32 strings](https://github.com/bitcoin/bips/blob/master/bip-0173.mediawiki), ![₿](img/bitcoin_32px.png) Bitcoin [“Bravo Charlie Addresses”](https://bitcointalk.org/index.php?topic=2646007.0), Bech32 .onion addresses...

- By nullius <[nullius@nym.zone](mailto:nullius@nym.zone)>
- PGP: [0xC2E91CD74A4C57A105F6C21B5A00591B2F307E0C](https://sks-keyservers.net/pks/lookup?op=get&search=0xC2E91CD74A4C57A105F6C21B5A00591B2F307E0C)
- Bitcoin, tips welcome: [3NULL3ZCUXr7RDLxXeLPDMZDZYxuaYkCnG](bitcoin:3NULL3ZCUXr7RDLxXeLPDMZDZYxuaYkCnG) (Segwit nested in P2SH), [bc1qcash96s5jqppzsp8hy8swkggf7f6agex98an7h](bitcoin:bc1qcash96s5jqppzsp8hy8swkggf7f6agex98an7h) (Segwit Bech32!).

Wait, did I say .onion addresses?  Yes, I think that it’s a good idea to add Bech32’s error-correcting code.

This utility’s actual encoding and decoding is done by [sipa’s Bech32 reference code](https://github.com/sipa/bech32/tree/master/ref/c), here included in tree.

For details, [RTFM](./bech32.1.md).  Yes, it has a manpage.  Software is unworthy of release if it does not have a proper manpage.

It has been tested on FreeBSD, my main platform, and on Linux.  [Unfortunately, I may have slightly mussed the BSD building while preparing for publication; this should soon be fixed.  The build system generally is still wonky.  This is an early release, with most attention paid to the source code and manpage!]

License: AVL v0.0.1 with Bitcoin consensus clause.  I would prefer to disclaim copyright, and and release things to the public domain (*the public domain is not a license, “CC0” people*).  However, this is not an ideal world.

Please direct usage discussion to the [forum thread](https://bitcointalk.org/index.php?topic=2664728.0), and bugs or concrete technical matters to the issue tracker.

## Installation

FreeBSD:

```
make && make check
```

...then, `make install` as root (via `sudo` or otherwise).  Other BSDs are probably similar.

Linux:

```
make -f Makefile.linux && \
	make -f Makefile.linux check && \
	sudo make -f Makefile.linux install
```
