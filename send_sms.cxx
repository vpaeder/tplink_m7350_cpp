/** \file send_sms.cxx
 *	This is a short example code that uses the TP-Link M7350 modem interface
 *	to send a SMS using the command line. This was tested to work with modem
 *	version 5 and firmware version 1.0.10.
 *	Author: Vincent Paeder
 *	License: GPL v3
 */
#include "tplink_m7350.h"
#include <iostream>
#include <unistd.h>

/* main function - returns 0 if execution went fine, 1 otherwise */
int main( int argc, char** argv ) {
	using namespace tplink;

	// flags indicating whether arguments have been set
	bool address_set = false, pw_set = false, number_set = false, message_set = false;
	// argument values
	std::string address, passwd, phone_number, message;

	// parse command line for arguments
	int opt;
	while ( ( opt = getopt ( argc, argv, "ha:p:n:m:" ) ) != -1 ) {
		switch ( opt ) {
			case 'h':
				std::cout << "Usage:" << std::endl;
				std::cout << argv[0] << " -a modem_address -p password -n phone_number -m message" << std::endl;
				std::cout << argv[0] << " -h" << std::endl;
				break;

			case 'a':
				address = optarg;
				address_set = true;
				break;

			case 'p':
				passwd = optarg;
				pw_set = true;
				break;

			case 'n':
				phone_number = optarg;
				number_set = true;
				break;

			case 'm':
				message = optarg;
				message_set = true;
				break;
		}
	}
	if (!address_set || !pw_set || !number_set || !message_set) {
		std::cout << "One of the required arguments has not been set." << std::endl;
		std::cout << "Type " << argv[0] << " -h for help" << std::endl;
		return 1;
	}

	TPLink_M7350 tpl;
	tpl.set_address(address);
	tpl.set_password(passwd);
	return tpl.send_sms(phone_number, message) ? 0 : 1;
}
