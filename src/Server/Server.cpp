#include "Server.h"

namespace http{

    #define CPU_CORES std::thread::hardware_concurrency()
    Server::Server() : m_bQuit(false), m_Listener(std::make_unique<Listener>()), m_CPUWorkers(std::make_unique<ThreadPool>(CPU_CORES)), m_APIWorkers(std::make_unique<ThreadPool> (4*CPU_CORES)){
        m_ServerThread = std::thread([this](){ this->run(); });
    }

    Server::~Server(){
        m_bQuit = true;
        if( m_ServerThread.joinable() )
            m_ServerThread.join();


        //welche reihenfolge?
        m_CPUWorkers.reset( nullptr );
        m_APIWorkers.reset( nullptr );
        m_Listener.reset( nullptr );
    }

    void Server::run(){
        while( !m_bQuit ) 
        {
            pollIncMessages();
            pollErrorMessages();

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void Server::pollIncMessages(){
        //könnten hier auch wieder ein limit machen dass errors nicht auf der strecke bleiben oder antowrten ode rso
        while ( true ) {
            {
                std::lock_guard<std::mutex> _lock (m_Queues.m_IncMsgMutex);
                if( m_Queues.m_IncomingMessages.empty() )
                    return;
            }
                
            //viel moven
            m_CPUWorkers->assignTask([]{});
        }

    }

    void Server::pollErrorMessages(){

    }
    //schritte:
    //parsen
    //api calls? / static gets -> Router
    //antwort formulieren
    //senden

    //mit move arbeiten wollen das ja nicht kopieren aber soweit ich weiß funktioniert das so mit moven gut
    void Server::parseRequest( Request httpRequest ){

    }
}