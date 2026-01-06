#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>

#include "Server/Server.h"

int main(){

	if( !http::initHTTP() ){
		std::cout << "couldnt initialize the lib" << "\n";
		exit(1);
	}

	http::Server s;
	//wenn init überhaupt gebracht aber mal schauen wie versuchen es zu vermeiden aber wenns sein muss mus sein. gleiche mit run  
	s.init();
	s.run( true );

	http::listenForCommands();
	
	return 0;
}
