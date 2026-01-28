#include "Server.h"

namespace http{

    Server::Server() : m_bQuit(false), m_Listener(std::make_unique<Listener>()){
        m_ServerThread = std::thread([this](){ this->run(); });
    }

    Server::~Server(){
        m_bQuit = true;
        if( m_ServerThread.joinable() )
            m_ServerThread.join();


        m_Listener.reset(nullptr);
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

    }

    void Server::pollErrorMessages(){

    }

    void Server::startListening( u_int16_t port ){

    }

    void Server::stopListening(){

    }
}