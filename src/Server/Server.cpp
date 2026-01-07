#include "Server.h"

namespace http{

    bool initHTTP(){
        //iwie so message am besten auch noch zurück geben oder so
        char a[1024]; 
        if( !GameNetworkingSockets_Init(nullptr, a) ){

            return false;
        }


        NetworkManager::Get().init();
    }

    bool HTTP_Kill(){
        //Alles runter fahren falls wir noch mehr brauchen
        GameNetworkingSockets_Kill();
    }

    void listenForCommands(){

    }

    Server::Server(){

    }

    Server::~Server(){

    }

    bool Server::init(){

    }

    void Server::run( bool startListeninig ){
        //aber run muss auch selbst auf eigenem thgread laufen kann ja unten eigene private methode haben die auf gquit wartet oder so aber merh übr threads lernen und so
        //std::thread(listener.listen) iwie so
        //Start listening and beeing open for connection
        //soll nicht direkt listening starten sondenr das soll manuell gemacht werden oder kann auch bool haben ob direkt gestartet werden soll 
    }
}