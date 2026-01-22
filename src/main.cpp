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
	//hoffentlich nicht
	s.init();
	
	s.run();

	//s.startListening( 80 );

	http::listenForCommands();

	http::HTTP_Kill();
	
	return 0;
}
