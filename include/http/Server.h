#pragma once

#include <memory>
#include <steam/steamnetworkingsockets.h>

#include "Datastrucutres/ThreadPool.h"
#include "NetworkManager.h"
#include "Parser.h"


//braucht möglichkeit einzustellen welche methoden man anbietet und welche nicht, z.B delet oder put dürfen manchmal ja einfach nith erlaubt sein
namespace http{
    class Server {
    public:
        void setFileRoot();
        void startListening( u_int16_t port ); 
        void stopListening(); 
    public:
        Server();
        Server(const Server& other) = delete;
        Server(Server&& other) = delete;
        ~Server();
    private:
        void run();
        void pollIncMessages();
        void pollErrorMessages();
    private:
        NetworkManager& m_rNetworkManager;
        std::unique_ptr<IParser> m_pParser;
        
        std::thread m_ServerThread;
        std::atomic<bool> m_bQuit = false;
        HListener m_Listener = HListener_Invalid;

        //Auslastung so monitoren und dann netowrk manager oder listener sagen request abzulehen bei überfüllung
        //länge von queues begrenzen 
        //länge von queues auch loggen
        std::unique_ptr<ThreadPool> m_CPUWorkers;
        std::unique_ptr<ThreadPool> m_APIWorkers;
    };
}
