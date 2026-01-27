#include "Server.h"

namespace http{

    Server::Server(){

    }

    Server::~Server(){

    }

    //sehr stark überlegen das init ab zuschaffen wird zu viel und warum? Ich meine listener auch von uniqe ptr weg machen wäre easy und beim erstellen einfach starten why not
    bool Server::init(){
        //Listener initialization in Konstruktor oder doch nur die init methode aufrufen
        m_Listener = std::make_unique<Listener> ();

        return true;
    }

    //brauchen ja eigenen server thread der dann auch errors polled. Muss er ja 
    //man muss explizit start listening aufrufen ganz wichtig
    void Server::run(){
        //aber run muss auch selbst auf eigenem thgread laufen kann ja unten eigene private methode haben die auf gquit wartet oder so aber merh übr threads lernen und so
        //std::thread(listener.listen) iwie so
        //Start listening and beeing open for connection
        //soll nicht direkt listening starten sondenr das soll manuell gemacht werden oder kann auch bool haben ob direkt gestartet werden soll 
    }
}