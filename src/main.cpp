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

	http::Server s;
	s.init();
	
	s.run( true );

	http::listenForCommands();
	
	return 0;
}
