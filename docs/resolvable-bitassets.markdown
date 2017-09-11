
Resolvable BitAssets
--------------------

Let us define a *resolvable BitAsset* as follows.  The BitAsset may be *resolved* (i.e., settled) based on the *feed resolution value* at a point in time called the *resolution date*.

The new BitAsset design presented by Dan today (4-13-2015) consists of a trio of BitAssets:  Two with standard bi-annual resolution dates, and a third equal to a portfolio of the first two assets.  The resolution price is equal to the simple moving average of the median feed value in a *resolution sampling window* consisting of the 30 days preceding the resolution date.

Declaration window
------------------

One improvement to resolvable BitAssets which comes to mind is to provide a *declaration window* whereby the long side must opt-in to the settlement process.  The declaration process would work something like this:  The *declaration window* consists of e.g. 60-45 days before the resolution date, each long asset holder may make a binding declaration stating that they wish to settle.  At the end of the declaration window, declaration causes each long holder's USD to be replaced by USDA, which is then settled on the resolution date.  This has the virtue of having only two assets (USD and USDA), and USDA only exists for the interval between the close of the declaration window and the resolution date.  There is no potential "A -> B conversion black swan event" which could potentially affect USD holders (although BitShares-style black swans where BTS drops like a rock, causing settlement to soar and exceed the total available collateral is still possible).

This creates a declare-wait-sample-resolve cycle consisting of the declaration window (60-45 days before resolution), a waiting period (45-30 days before resolution), a sampling window (30-0 days before resolution), and resolution.  The waiting period is necessary to deal with the fact that the price feed will be slow to respond to new information in the market, thus some lag period provides a "handicap" of sorts to "hobble" the traders who would normally be able to move faster than the system and outmaneuver it.  Note that since the USDA is transferrable and tradeable during the waiting period and sampling period, USDA holders will likely be able to find counterparties in the market willing to convert their USDA to BTS right away (at a small spread of course).  So all of the long side's wealth is always liquid.

Equitable shorting
------------------

Another modification which I have been thinking about for a very long time is *equitable shorting*, which does away with the traditional shorting mechanics.  Equitable shorting divides the short side into *short shares* which are fully tradeable and fungible.  Unlike BitShares-style shorting, all short positions have equal debt-to-collateral ratios in equitable shorting, and short positions are naturally divisible (to the satoshi), transferrable, tradeable, and fungible.  Equitable shorting offers substantial bookkeeping advantages, and the mechanics are much easier to explain than shorting.  Furthermore, depending on the details of the mechanics, it may be possible to design equitable shorting in a manner that separates (at least to some extent) *market* activities (involving counterparties making deals with each other) from *supply* activities (involving creation or redemption of long/short shares).

Balanced supply operations
--------------------------

At any time, identical "balanced" fractions of the long and short float can be created or redeemed, indeed without any reference whatsoever to any "book value" of the long-vs-short split.  So for example, if 10 long shares backed by 100 BTS are outstanding, and 20 short shares backed by 200 BTS are also outstanding, then creating a basket of 1 long share and 2 short shares will increase the float by 10% and thus require BTS equal to 10% of the existing collateral to create the backing.  Thus the collateral for the new basket is 30 BTS.  After the basket is created, there will be 11 long shares backed by 110 BTS, and 22 short shares backed by 220 BTS.

Let's re-run that example with the same total collateral but a very different price -- the long side is 10 long shares backed by 200 BTS and 20 short shares backed by 100 BTS.  At this valuation, creating 1 long share and 2 short shares backed by a total of 30 BTS results in 11 long shares backed by 220 BTS and 22 short shares backed by 110 BTS.

This shows that regardless of what is the "correct" price, balanced creation (or redemption) doesn't change anyone's equation and is therefore "economically safe" to allow a single actor to perform, since it doesn't have effects of increasing risk or transferring wealth on any third party.

Unbalanced supply operations
-----------------------------

There must be a way for new investors to re-capitalize a losing short side, and a way for old investors to take profits from a winning short side.  In other words, while it does provide liquidity in both directions, balanced creation/redemption preserves the debt-to-collateral ratio of the float, and thus will be unable to correct any deviation from the ideal 300% backing.  However, unbalanced supply operations need a notion of *book value*.  If 20 short shares are backed by 200 BTS, then creating a short share (without creating any corresponding long share) should cost 10 BTS (which will go into the backing).  If those 20 shares are backed by 100 BTS instead, then creating a short share (again, without any corresponding long share) should cost 5 BTS!

Unbalanced supply operations on the short side are necessary, but suffer from the same problem as unbalanced long-side operations.  Thus, a similar solution seems appropriate -- use a declare-wait-sample-resolve cycle.  However this cycle should be much shorter than what we contemplated above for the long side.  If the price is dropping and the short side is in danger of reaching black swan territory, but there are plenty of investors willing to short, forcing them to wait 30 days seems perverse.  Indeed the DWSR cycle can be shortened, the limitation is how quickly pricing information known to investors moves into the price feed.  A daily DWSR cycle with 16/2/6 hours allowing unbalanced creation of shorts when collateralization is less than 300% seems reasonable.

When collateralization is greater than 300%, instead of admitting new unbalanced shorts, the DWSR cycle should instead pay the excess collateral to the shorts.  This is a combination of taking profit and liberation of excess capital (the short side can also become overcollateralized in the case where the long side is shrinking through redemptions).

Creating new longs
------------------

Conditions where less than 300% collateral exists and existing longs are illiquid may be eased if market makers are allowed to create new longs.  Thus, we can initiate a simple rule:  New shorts must be issued with new longs such that the new longs are collateralized 300%.  If the existing collateralization is less than 300%, this results in a mix of balanced creation and unbalanced short creation.  If the existing collateralization is more than 300%, then this results in a mix of balanced creation and unbalanced long creation.  In either case, the new short causes the overall market collateralization moves toward 300%.

Equitable "margin call"
-----------------------

A *margin call* is requiring the short to buy out the long (i.e. debt) at a premium to book value when collateralization is getting dangerously close to 100%, in order to reduce the probability of a black swan.

In BitShares, margin call happens on a position-by-position basis, and the excess collateral liberated by the margin call is cashed out.  With equitable shorts, it is possible to let this liberated collateral remain to shore up the collateralization of the remaining long shares.

Reducing long DWSR cycles
-------------------------

Upon consideration of the DWSR cycle for shorts, there is little reason the long DWSR cycle cannot be reduced to the same time interval.  A reduced DWSR cycle for longs has the benefit of improving the symmetry between long and short (elegance and perceptions of fairness (i.e. the system doesn't unduly favor shorts or longs)).  It allows market makers the ability to trade with greater velocity, improving their returns, and some of this benefit will be passed on to the markets they serve in the form of better liquidity and lower spreads.

The best benefit of a reduced long DWSR cycle is it will allow us to get rid of USDA.  The main reason USDA exists is to keep a redeeming long holder liquid during the WS parts of the DWSR cycle.  If the WS part of the cycle is 8 hours instead of 30 days, the loss of liquidity is much less harmful to long holders.  There may be a decent number of traders willing to pay fees and spreads to exit a position 30 days early, but there are relatively few willing to pay fees and spreads to exit a position 8 hours early (and those who are willing to do so still have the alternative of selling their USD to a market maker instead of redeeming it).

Rules
-----

- (R1) Can only do unbalanced creation when books are empty
- (R2) Unbalanced short creation only happens once per day (I assume this happens during a maintenance block at midnight, 00:00 UTC)
- (R3) Unbalanced short creation must be declared by 16:00 UTC the previous day
- (R4) Unbalanced short creation uses sampling window of 18:00 UTC - 00:00 UTC
- (R5) Price feed providers should update once per hour
- (R6) Unbalanced long creation requires 2x short creation
- (R7) Balanced redemption is possible at any time without waiting (analogous to short cover)
- (R8) Balanced creation is possible at any time without waiting, provided overall collateralization is 250% or more
- (R9) Margin call begins at 200% collateralization
- (R10) Margin call pays premium which increases smoothly from 0% at 200% collateralization, to 10% at 150% collateralization.
- (R11) If collateralization exceeds 300%, the excess is returned to shorts, quantized to 0.1% of the book value.
- (R12) The check in R11 is performed in a maintenance block after long redemption, but before short creation.

Example
-------

First, Alice creates a basket of short/long shares.  This is considered unbalanced creation since there is no existing market (R1).  She uses 7500 BTS to do this.  She must
declare her intent by 16:00 UTC (R3), upon her declaration the 7500 BTS become locked.  At 00:00 UTC the next day, a book price is determined by the average value of
the median feed from 18:00 UTC to 00:00 UTC (R4), let us say this value is $0.04 USD / BTS.  1/3 of the pledged collateral goes to the long side, 2/3 goes to the short side.
The number of long shares issued will be determined by the price.  2500 BTS at $0.04 USD / BTS has a value of $100, thus 100 long shares are created, and 200 short shares are
also created.

To simplify discussion, let's say Alice transfers the long shares to Lisa.  (Long holders will have names starting with letters "L" and later, short holders will begin with "A".  A list
of alphabetical names is available in BitShares at https://github.com/BitShares/bitshares/blob/3ebe73b463d637d3c0e991d6abd4613b68bed3b6/tests/drltc_tests/tscript/genesis.py#L31 ).
We have these books:

    -------------------------------------------
    | CR: 7500 / 2500 = 300%                  |
    | PF: 0.04 USD / BTS                      |
    -------------------------------------------------------------------------------------------
    | short      shares              equity   |  long        shares                  equity   |
    -------------------------------------------------------------------------------------------
    | Alice      200 SHORT_USD    5000 BTS    |  Lisa        100 USD               2500 BTS   |
    -------------------------------------------------------------------------------------------
    | (total)    200 SHORT_USD    5000 BTS    |  (total)     100 USD               2500 BTS   |
    -------------------------------------------------------------------------------------------

Note that equity is a bookkeeping fiction based on the price feed value.  We "want" Lisa to be able
to redeem her USD for 2500 BTS at any time.  However, Lisa may know things about the price that are
not reflected in the feed (e.g. by directly observing the exchanges that feed value is based on).
If Lisa knows the value of the feed is about to fall, then instead of waiting for the shoe to drop,
she might try to "catch the shoe" and redeem her USD before the feed catches up to the exchange.  The
DWSR cycle's purpose is to prevent this maneuver.

The price feed falls to $0.035 BTS.  The books now look like this:

    -------------------------------------------
    | CR: 7500 / 2857 = 263%                  |
    | PF: 0.035 USD / BTS                     |
    -------------------------------------------------------------------------------------------
    | short      shares              equity   |  long        shares                  equity   |
    -------------------------------------------------------------------------------------------
    | Alice      200 SHORT_USD    4643 BTS    |  Lisa        100 USD               2857 BTS   |
    -------------------------------------------------------------------------------------------
    | (total)    200 SHORT_USD    4643 BTS    |  (total)     100 USD               2857 BTS   |
    -------------------------------------------------------------------------------------------

Let's say Bob uses 3000 BTS to enter a DWSR cycle which resolves with a price of $0.035,
and transfers the long side to Matt.  The new longs are collateralized with 1000 BTS, and the new shorts
are collateralized with 2000 BTS.  The number of short shares Bob gets is based on the value of his short
shares (2000 BTS) over the short side's new capital pot (6643 BTS).

    -------------------------------------------
    | CR: 10500 / 3857 = 272%                 |
    | PF: 0.035 USD / BTS                     |
    -------------------------------------------------------------------------------------------
    | short      shares              equity   |  long        shares                  equity   |
    -------------------------------------------------------------------------------------------
    | Alice      200 SHORT_USD    4643 BTS    |  Lisa        100 USD               2857 BTS   |
    | Bob         86 SHORT_USD    2000 BTS    |  Matt         35 USD               1000 BTS   |
    -------------------------------------------------------------------------------------------
    | (total)    286 SHORT_USD    6643 BTS    |  (total)     135 USD               3857 BTS   |
    -------------------------------------------------------------------------------------------

As you can see, the collateral ratio increased.

If the price moved back to 0.040 USD / BTS, what would happen?

    -------------------------------------------
    | CR: 10500 / 3857 = 272%                 |
    | PF: 0.040 USD / BTS                     |
    -------------------------------------------------------------------------------------------
    | short      shares              equity   |  long        shares                  equity   |
    -------------------------------------------------------------------------------------------
    | Alice      200 SHORT_USD    4983 BTS    |  Lisa        100 USD               2500 BTS   |
    | Bob         86 SHORT_USD    2142 BTS    |  Matt         35 USD                875 BTS   |
    -------------------------------------------------------------------------------------------
    | (total)    286 SHORT_USD    7125 BTS    |  (total)     135 USD               3375 BTS   |
    -------------------------------------------------------------------------------------------

This book shows a very interesting phenomenon.  Alice's equity was originally 5000 BTS at 0.040 USD / BTS, and
she didn't engage in any transactions; from her point of view, the only things which occurred were a price
feed move and trading by third parties.  What is going on here?  Have we discovered an attack vector?  Who has
taken Alice's money?

The answer is that Bob has received the extra value.  Bob's action can be viewed as a combination of balanced creation
of 35 long shares backed by 1000 BTS and 70 short shares backed by 1630 BTS, followed by an unbalanced creation of
16 short shares backed by 370 BTS.

Unbalanced creation of short shares has the effect of de-leveraging every other short holder.  Thus, Alice really experiences
a loss of leverage, rather than loss of capital.  Indeed Alice's balance sheet is unchanged immediately before and after Bob's
entrance -- leverage only matters when the price is moving.  This effect cuts both ways:  If the price moved in the opposite
direction, causing losses to the shorts, then Alice would experience a smaller loss.

Alice shorted, the price feed moved against her, she was de-leveraged when Bob entered the market, then the price feed regained
its ground.  This story makes it clear how Alice's value was lost.  If the price feed had moved further against the shorts, instead
of regaining ground, Alice would have benefited instead; Bob would have taken some of the losses that would have otherwise been assigned
to Alice.
