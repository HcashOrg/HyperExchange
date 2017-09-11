
Creating a new FBA involves some manual steps, as well as a hardfork which sends the fees to the FBA.

- Create the FBA asset issuer account (or use existing account).
- Create the FBA asset.
- Issue FBA initial distribution.
- If decentralized governance of the asset is desired, FBA issuer uses the `account_update_operation` with  `owner_special_authority` and `active_special_authority` extensions to set its owner/active authorities to `top_holders_special_authority`. #516
- FBA issuer creates buyback account for FBA with `account_create_operation` including `buyback_options` extension. #538

Note, the steps with issue numbers cannot be done until the corresponding hardfork date has passed, which (as of this writing) is planned for February 2016.

The asset ID of the FBA is then included in the hardfork which implements the fee redirection.  Fees are then directed to the asset's buyback account, provided the asset has a buyback account; otherwise the fees go to the network.  (So in the case of the first FBA in February 2016, when the hardfork to enable the above-mentioned extensions launches, any fees gathered between the redirection hardfork and the buyback account being created will go to the network.  For this reason perhaps the redirection hardfork should be ~3 hours behind the extension hardforks to allow time for the final manual setup.)
