
Status
------

This is my (@theoreticalbts) getting some ideas down on paper regarding DPOS.  It's not really up-to-date with my latest thinking.  So anyone reading this shouldn't take it as a serious proposal -- it's more me thinking out loud.

DPOS slot tickets
-----------------

Now that we are re-visiting the DPOS algorithm for Graphene, I (@theoreticalbts) would like to propose a modification to decentralize with dynamic delegates.  The core of this proposal is an algorithm which results in a delegate having a chance to produce a block proportional to their *support* (the amount of stake voting for them).

- Delegates can have planned downtime without any protocol modifications
- No fixed number of delegates, fully decentralized
- The most popular delegates produce most blocks, but even highly unpopular delegates occasionally get to produce a block
- Delegate ordering can be fixed several minutes ahead of time
- Efficient implementation via auction protocol
- Delegates can be required to post a bond which is only returned when they actually publish a block, make downtime unprofitable without huge block rewards

A delegate which produces a block becomes ineligible to produce another block for some cooldown time.

Details
-------

At block height `h`, each node which controls a delegate object locally produces a *ticket*.  The ticket represents a chance to produce a block of height between `h+c` and `h+d` for some blockchain config parameters `c`, `d` (e.g. `c = 384` and `d = 416`).

The ticket's *priority* is computed as `priority = H(random_state, delegate_id, parent_block_hash) * f(support)` where `support` is the votes for the account, `random_state` is a strong random state (no single previous delegate controls more than a single bit).  The highest priority wins.  The choice of the *advantage function* `f()` will be discussed in the next section.

Tickets of priority `2**(N+64)` or more, where `N` is the number of bits in the hash function, may be published between height `h` and `h+a`.  The worst priority of publishable tickets then falls exponentially until `h+b`, until all tickets are allowed to be published between `h+b-a` and `h+b` (where `a` and `b` are more config parameters, e.g. `a = 128`, `b = 256`).  No tickets are allowed to be published between `h+b` and `h+c`.

Then for each future slot height, the slot is assigned to the ticket with the winning eligible priority.  A delegate assigned to the block at height `z` will become ineligible until `z+g` blocks have passed, where `g >= d-c` is another config parameter.  (The restriction `g >= d-c` implies that no ticket can be re-used.)
Here is a diagram:

    produce                better                  ticket was eligible         ticket was eligible         ticket becomes            ticket stops being      earliest time when             latest time when
    ticket                 tickets                 to publish for at least     to publish during some      eligible to produce       eligible to produce     delegate would become          delegate would become
    |                      start                   one block regardless        block window of length      a block                   a block                 eligible to produce            eligible to produce
    |                      to become               of its value                a regardless of prio             |                          |                 another block                  another block
    |                      unpublishable                  |                          |                          |                          |                 (with another ticket)          (with another ticket)
    |                          |                          |                          |                          |                          |                          |                          |
    |   --->--->--->--->--->   |   --->--->--->--->--->   |                          |    waiting period        |                          |    waiting period        |   --->--->--->--->--->   |
    |       worse tickets      |       worse tickets      |                          |    makes it harder       |                          |    dictates at what      |      tickets later       |
    |     gradually become     |     gradually become     |                          |    to game RNG           |                          |    point support         |   in the window          |
    |        publishable       |        publishable       |                          |                          |                          |    "saturates" and       |   gradually become       |
    |                          |                          |                          |                          |                          |    additional votes      |   eligible to produce    |
    |                          |   --->--->--->--->--->   |   --->--->--->--->--->   |                          |                          |    no longer increase    |   again                  |
    |                          |      better tickets      |      better tickets      |                          |                          |    block production      |                          |
    |                          |     gradually become     |     gradually become     |                          |                          |    frequency             |                          |
    |                          |       unpublishable      |       unpublishable      |                          |                          |                          |                          |
    |                          |                          |                          |                          |                          |                          |                          |
    |                          |                          |                          |                          |                          |                          |                          |
    |                          |                          |                          |                          |                          |                          |                          |
    |                          |                          |                          |                          |                          |                          |                          |
    |                          |                          |                          |                          |                          |                          |                          |
    h                         h+a                       h+b-a                       h+b                        h+c                        h+d                       h+c+g                      h+d+g

If `d-c` better tickets have been published, the delegate should throw away his ticket to save the publication fee, since he'll be unable to win.

As well as a publication fee, a ticket may include a bond requirement, some amount of stake (e.g. 9x the block reward) which is only refunded if the block is produced (or the ticket expires at height `h+d` without being used to produce a block).  A 9x bond would make it unprofitable to run a delegate that has less than 90% uptime.

Delegates should include their secret hash with their ticket.

Delegates that are offline are automatically dropped.  All nodes have an opportunity to become a delegate.

High priority tickets are only allowed to be published for a window in order to prevent strategically withholding tickets until the last minute.  Each ticket has an `a` block wide publication window where it must be revealed in order to be valid.  The process is similar to a "downward" auction where the auctioneer names a very high figure that is highly unlikely to be paid, then exponentially lowers the bidding level until enough buyers enter to buy all the product for sale.  The "bidding" here is not through anything with permanent value, but rather ephemeral priority.

The advantage function
----------------------

The role of `f()` determines how much support affects the probability of being voted in.  It is clear that `f()` should be monotonically increasing (greater support always increases probability of a winning ticket).

Suppose we two delegates `D` and `D'` with support `v` and `v'` where `v >= v'`.  Let us compute the probability that `priority(D) >= priority(D')`.  A delegate `D` ticket is uniformly chosen from `[0, f(v))` and a delegate `D'` ticket is uniformly chosen from `[0, f(v'))`.  Delegate `D` wins if its ticket contains a number greater than `f(v')`, which happens with probability `q = (f(v) - f(v')) / f(v)`.  In the remaining `1 - q` cases, `D` is uniformly in `[0, f(v'))` which means it has a `50%` chance of winning.  Thus the overall probability of winning, `p`, is:

    p = q + (1 - q) / 2
      = q + 1/2 - q/2
      = (q+1) / 2
      = ((f(v) - f(v')) / f(v) + 1) / 2
      = (2*f(v) - f(v')) / (2 * f(v))
      = 1 - f(v') / (2*f(v))

Now suppose we want the probability of being chosen to be "linear".  This means that we want `p / (1-p) = v / v'`.

    p / (1-p) = (1 - f(v') / (2*f(v))) / (f(v') / (2*f(v)))
              = 2*f(v) / f(v') - 1

Some numerical experimentation shows that the following choice of `f`, which I will denote `F`, results in "linear" probability:

    f(v) = F(v) = (v-1) / 2 + 1

