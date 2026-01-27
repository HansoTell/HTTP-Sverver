#pragma once

#include <steam/steamnetworkingsockets.h>
#include <queue>

#include "../Listener/NetworkManager.h"
#include "../Listener/listener.h"
#include "Queues.h"


namespace http{


    class Server {
    public:
        //init aller module aufrufen
        bool init();
        //soll auf anderem thrad laufen ig also soll nicht blockieren der server
        void run();
        void setFileRoot();

    public:
        Server();
        Server(const Server& other) = default;
        Server(Server&& other) = default;
        ~Server();
    public:

    private:
        void pollErrorQueue();

    private:
        MessageQueues m_Queues;
        //vielleicht sogar global variable also wird schon sehr viel gebraucht
        bool quit = false;
        std::queue<const char*> messageQueue;
        std::unique_ptr<Listener> m_Listener;
    };

}