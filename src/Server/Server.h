#pragma once

#include <steam/steamnetworkingsockets.h>
#include <queue>

#include "../Listener/NetworkManager.h"
#include "../Listener/listener.h"
#include "Queues.h"


namespace http{


    class Server {
    public:
        void setFileRoot();
        void startListening( u_int16_t port );
        void stopListening();
    public:
        Server();
        Server(const Server& other) = default;
        Server(Server&& other) = default;
        ~Server();
    public:

    private:
        void run();
        void pollIncMessages();
        void pollErrorMessages();

    private:
        std::thread m_ServerThread;
        std::atomic<bool> m_bQuit = false;

        MessageQueues m_Queues;
        std::unique_ptr<Listener> m_Listener;
    };

}