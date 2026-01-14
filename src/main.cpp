#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>

#include "Server/Server.h"
#include "Server/HTTPinitialization.h"

int main(){

	if( !http::initHTTP() ){
		std::cout << "couldnt initialize the lib" << "\n";
		exit(1);
	}

	//http::Server s;
	//s.init();
	
	//s.run( true );

	//http::listenForCommands();

	LOG_DEBUG("DEBUG Test");
	LOG_INFO("Info Test");
	LOG_WARNING("Warning test");
	LOG_ERROR("Error Test");
	LOG_CRITICAL("Critical Test");

	http::g_Logger->DEBUG("Debug Test 2");
	http::g_Logger->INFO("INFO Test 2");
	http::g_Logger->WARNING("WARNING Test 2");
	http::g_Logger->ERROR("ERRO Test 2");
	http::g_Logger->CRITICAL("CRITICAL Test 2");

	Error::ErrorValue<http::HTTPErrors> testError = {http::HTTPErrors::bsp, "Huso", CURRENT_LOCATION };

	LOG_VDEBUG("Test Error Variable", 42, testError, MAKE_ERROR(http::HTTPErrors::bsp, "Makro Test"));

	std::cin.get();

	http::HTTP_Kill();
	
	return 0;
}
