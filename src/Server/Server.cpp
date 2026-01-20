#include "Server.h"

namespace http{

    Server::Server(){

    }

    Server::~Server(){

    }

    bool Server::init(){
        //Listener initialization in Konstruktor oder doch nur die init methode aufrufen
        m_Listener = std::make_unique<Listener> ();

        return true;
    }

    void Server::run( bool startListeninig ){
        //aber run muss auch selbst auf eigenem thgread laufen kann ja unten eigene private methode haben die auf gquit wartet oder so aber merh übr threads lernen und so
        //std::thread(listener.listen) iwie so
        //Start listening and beeing open for connection
        //soll nicht direkt listening starten sondenr das soll manuell gemacht werden oder kann auch bool haben ob direkt gestartet werden soll 
    }
}