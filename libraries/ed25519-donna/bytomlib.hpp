#pragma once

#include <string>
namespace bytom {
	bool ValidateAddress(std::string address, std::string NetParam);
	std::string GenerateBytomKey(std::string memstr);
	std::string GetBytomPubKeyFromPrv(std::string Xprev);
	std::string GetBytomAddressFromPub(std::string Pubkey, std::string NetParam = "mainnet");
	std::string GetBytomAddressFromHash(std::string ctrlProgHash, std::string NetParam = "mainnet");
	std::string SignMessage(std::string hex_str, std::string Xprev) ;
	bool VerifySignBytomMessage(std::string sign_str, std::string hex_source_msg, std::string Xpub);
}