#pragma once

#include <steam/steamnetworkingsockets.h>
#include <queue>

#include "../Listener/NetworkManager.h"


namespace http{

    //steam library initen und so
    extern bool initHTTP();

    //was erschaffen wird muss auch getötet werden
    extern bool HTTP_Kill();

    //blockende methode die nach nutzereingaben guckt und die dann an den server als commands weitergibt einfach in die message queue gepackt iwie so
    extern void listenForCommands();

    class Server {
    public:
        //init aller module aufrufen
        bool init();
        //soll auf anderem thrad laufen ig also soll nicht blockieren der server
        void run( bool startListening = false );
        void setFileRoot();

    public:
        Server();
        Server(const Server& other) = default;
        Server(Server&& other) = default;
        ~Server();

    private:
        //vielleicht sogar global variable also wird schon sehr viel gebraucht
        bool quit = false;
        std::queue<const char*> messageQueue;
    };

}