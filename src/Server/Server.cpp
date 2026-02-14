#include <http/Server.h>
#include <optional>

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
        #define REQUEST_LIMIT_PER_SESSION 100
        u_int16_t limitCounter = 0; 

        while( limitCounter < REQUEST_LIMIT_PER_SESSION )
        {
            std::optional<Request> msg = m_Listener->m_RecivedMessegas.try_pop();
            if( !msg.has_value() )
                break;

            m_CPUWorkers->assignTask([this, &msg]()  {
                this->parseRequest(std::move( msg.value() ));
            });
            limitCounter++;
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
    //muss eh in ein anderes modul da kann man sich dann gedaniken machen
    void Server::parseRequest( Request httpRequest ){

    }
}
