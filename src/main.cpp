#include <iostream>
#include <stdint.h>

#include "http/HTTPinitialization.h"
#include <http/Server.h>

int main(){

	if( !http::initHTTP() ){
		std::cout << "couldnt initialize the lib" << "\n";
		exit(1);
	}

	http::Server s;

	s.startListening( 80 );

	http::listenForCommands();

	http::HTTP_Kill();
	
	return 0;
}
