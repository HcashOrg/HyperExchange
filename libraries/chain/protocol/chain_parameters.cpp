#include <graphene/chain/protocol/chain_parameters.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>
#include <fc/io/raw.hpp>




namespace graphene {
	namespace chain {
		chain_parameters::chain_parameters() {
			current_fees = std::make_shared<fee_schedule>();
		}

		// copy constructor
		chain_parameters::chain_parameters(const chain_parameters& other)
		{
			current_fees = std::make_shared<fee_schedule>(*other.current_fees);
			safe_copy(*this, other);
		}

		// copy assignment
		chain_parameters& chain_parameters::operator=(const chain_parameters& other)
		{
			if (&other != this)
			{
				current_fees = std::make_shared<fee_schedule>(*other.current_fees);
				safe_copy(*this, other);
			}
			return *this;
		}

		void chain_parameters::safe_copy(chain_parameters& to, const chain_parameters& from)
		{
			to.block_interval = from.block_interval;
			to.maintenance_interval = from.maintenance_interval;
			to.maintenance_skip_slots = from.maintenance_skip_slots;
			to.committee_proposal_review_period = from.committee_proposal_review_period;
			to.maximum_transaction_size = from.maximum_transaction_size;
			to.maximum_block_size = from.maximum_block_size;
			to.maximum_time_until_expiration = from.maximum_time_until_expiration;
			to.maximum_proposal_lifetime = from.maximum_proposal_lifetime;
			to.maximum_asset_whitelist_authorities = from.maximum_asset_whitelist_authorities;
			to.maximum_asset_feed_publishers = from.maximum_asset_feed_publishers;
			to.maximum_miner_count = from.maximum_miner_count;
			to.maximum_guard_count = from.maximum_guard_count;
			to.minimum_guard_count = to.minimum_guard_count;
			to.maximum_authority_membership = from.maximum_authority_membership;
			to.reserve_percent_of_fee = from.reserve_percent_of_fee;
			to.network_percent_of_fee = from.network_percent_of_fee;
			to.lifetime_referrer_percent_of_fee = from.lifetime_referrer_percent_of_fee;
			to.cashback_vesting_period_seconds = from.cashback_vesting_period_seconds;
			to.cashback_vesting_threshold = from.cashback_vesting_threshold;
			to.count_non_member_votes = from.count_non_member_votes;
			to.allow_non_member_whitelists = from.allow_non_member_whitelists;
			to.miner_pay_per_block = from.miner_pay_per_block;
			to.miner_pay_vesting_seconds = from.miner_pay_vesting_seconds;
			to.worker_budget_per_day = from.worker_budget_per_day;
			to.max_predicate_opcode = from.max_predicate_opcode;
			to.fee_liquidation_threshold = from.fee_liquidation_threshold;
			to.accounts_per_fee_scale = from.accounts_per_fee_scale;
			to.account_fee_scale_bitshifts = from.account_fee_scale_bitshifts;
			to.max_authority_depth = from.max_authority_depth;
			to.minimum_pledge_weight_line = from.minimum_pledge_weight_line;
			to.minimum_guard_pledge_line = from.minimum_guard_pledge_line;
			to.bonus_distribute_limit = from.bonus_distribute_limit;
			to.min_pay_back_balance = from.min_pay_back_balance;
			to.min_pay_back_balance_other_asset = from.min_pay_back_balance_other_asset;
			to.validate_time_period = from.validate_time_period;
			to.extensions = from.extensions;
		}
		chain_parameters::chain_parameters(chain_parameters&& other)
		{
			current_fees = std::move(other.current_fees);
			safe_copy(*this, other);
		}

		// move assignment
		chain_parameters& chain_parameters::operator=(chain_parameters&& other)
		{
			if (&other != this)
			{
				current_fees = std::move(other.current_fees);
				safe_copy(*this, other);
			}
			return *this;
		}

	}
}