
These are possible features I (@theoretical) want to discuss.

- Saleable / transferrable short positions

Short position should be its own object which is transferrable between accounts.  Including "for sale" flag allowing it to be atomically traded for a certain amount of asset.

NB it is similar to real estate market, every short position is unique due to having its own debt-to-collateral ratio.

- Short "I want in" or "I want out"

In BitAssets 3.0, shorts should be allowed to specify a flag indicating whether they want to stay in (as much as possible) or whether they want to stay out.  If a short wants in, on redemption they will be partially covered; if a short wants out, they will be fractionally covered.

- Spread options positions

A long option (e.g. allowing you to buy) can be combined with a short option (e.g. requiring you to sell), then instead of providing the position you might be required to sell, you only have to provide the capital to exercise the long.  Hmm, could this create unbounded load via a chain of option exercises?

- Privatized BitAssets : Separate feed providers from asset owners

Providing a feed is simple, providing an asset is a business which has to be concerned with a specific type of marketing and user acquisition.  Provide a way for some organization whose only business is publication of a feed a way to do so, and let anyone else who wants to be in charge of creating and marketing an asset that uses that feed.

Feed providers can have their own branding, there are plenty of organizations that publish highly trusted feeds in the real world, they might be interested in monetizing their feed by putting it on our platform (e.g. if WSJ wants to be a feed provider, then someone else would be able to have an asset that is settled based on WSJ numbers.  WSJ gets a portion of the fees in exchange for allowing asset to use their brand).  Technically there is no way to prevent "stripping", e.g. someone else publishes a reduced-fee or free feed making a feed equal to the WSJ feed, but people would have to trust the stripper to always remain faithful.  It's easier to trust the WSJ than it is to trust some random obscure businessman who promises to always mirror the WSJ; we'd be providing WSJ a way to extract the premium people are willing to pay for the trust in their brand.

- Graduated margin call premium

Arhag mentions graduated margin call premium here:  https://bitsharestalk.org/index.php/topic,15775.msg202705.html#msg202705

- Market matching of redemption requests

Despite my rebuttal of arhag here:  https://bitsharestalk.org/index.php/topic,15775.msg202832.html#msg202832

It turns out that we can actually do market matching of redemption requests.  If there's a party who's willing to give the long what they're asking for voluntarily, we should provide the long what they ask for by matching with that party, rather than forcing a short to settle involuntarily.

- Stochastic redemption time

Instead of redeeming during a maintenance block, a redemption order has a chance of executing each block.  The chance is 0% for the first 24 hours (giving the feed time to catch up), then rises gradually to 100% at 48 hours according to some curve (finding a good curve is an interesting problem).
