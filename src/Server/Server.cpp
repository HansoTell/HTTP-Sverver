#include "Server.h"

namespace http{

    bool initHTTP(){
        //Steam netowking library initiallisieren und so weiter alles einrichten
    }

    void listenForCommands(){

    }

    Server::Server(){
        //run callen?
        //steam library initen
        //denke alles müsste auf einem seperatem thread laufen und im hintergrund also sollte threat mit run starten
        //oder machen das in eine init methode
        //auf jeden fall alle librarys initen.
    }

    Server::~Server(){
    }

    bool Server::init(){
        //listener init callen
    }

    void Server::run(){
        //aber run muss auch selbst auf eigenem thgread laufen kann ja unten eigene private methode haben die auf gquit wartet oder so aber merh übr threads lernen und so
        //std::thread(listener.listen) iwie so
        //Start listening and beeing open for connection
        //
    }
}