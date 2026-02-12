#pragma once

#include <steam/steamnetworkingsockets.h>

#include "../Listener/listener.h"
#include "Request.h"
#include "ThreadPool.h"


namespace http{


    class Server {
    public:
        void setFileRoot();
        void startListening( u_int16_t port, const char* socketName = nullptr ) { m_Listener->startListening( port, socketName ); }
        void stopListening() { m_Listener->stopListening(); }
    public:
        Server();
        Server(const Server& other) = delete;
        Server(Server&& other) = delete;
        ~Server();
    public:

    private:
        void run();
        void pollIncMessages();
        void pollErrorMessages();

        void parseRequest( Request httpRequest );

    private:
        std::thread m_ServerThread;
        std::atomic<bool> m_bQuit = false;

        std::unique_ptr<Listener> m_Listener;

        //Auslastung so monitoren und dann netowrk manager oder listener sagen request abzulehen bei überfüllung
        //länge von queues begrenzen 
        //länge von queues auch loggen
        std::unique_ptr<ThreadPool> m_CPUWorkers;
        std::unique_ptr<ThreadPool> m_APIWorkers;
    };

}
