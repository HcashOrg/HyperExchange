#include <bytomlib.hpp>
#include <iostream>
#include <vector>

int
main(void) {
	std::string new_pri = bytom::GenerateBytomKey("");
	std::string new_pub = bytom::GetBytomPubKeyFromPrv(new_pri);
	std::string sig = bytom::SignMessage("", new_pri);
	std::cout << sig << std::endl;
	
	
	return 0;
}