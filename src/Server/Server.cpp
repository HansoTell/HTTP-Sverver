#include "Error/Errorcodes.h"
#include "http/NetworkManager.h"
#include <http/Server.h>
#include <optional>
#include <sys/types.h>


namespace http{

    #define CPU_CORES std::thread::hardware_concurrency()
    Server::Server() 
            : m_bQuit(false), m_CPUWorkers(std::make_unique<ThreadPool>(CPU_CORES)),
                m_APIWorkers(std::make_unique<ThreadPool> (4*CPU_CORES)), m_rNetworkManager(NetworkManager::Get())
    {
        m_Listener = m_rNetworkManager.createListener(nullptr)  ;
        
        m_ServerThread = std::thread([this](){ this->run(); });
    }

    Server::~Server(){
        m_bQuit = true;
        if( m_ServerThread.joinable() )
            m_ServerThread.join();

        //welche reihenfolge?
        m_CPUWorkers.reset( nullptr );
        m_APIWorkers.reset( nullptr );
        m_rNetworkManager.DestroyListener( m_Listener ) ;
    }

    void Server::run(){
        while( !m_bQuit ) 
        {
            pollIncMessages();
            pollErrorMessages();

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }


    void Server::startListening( u_int16_t port ){
        auto erg = m_rNetworkManager.startListening( m_Listener, port);
        
        if( !erg.isErr() ){
            return;
        }

        if ( erg.error().ErrorCode == HTTPErrors::eInvalidListener ){
            //Können wir einfach neu starten ich meine wenn der listener nicht da ist dann brauchen wir neuen
            //Aber alle alten nachrichten können mehr raus geschikt werden
        }
    
        if( erg.error().ErrorCode == HTTPErrors::eSocketInitializationFailed ){
        
        }
    }

    void Server::stopListening(){
        
    }

    void Server::pollIncMessages(){
        #define REQUEST_LIMIT_PER_SESSION 100
        u_int16_t limitCounter = 0; 

        auto pMessages = NetworkManager::Get().getQueue<Request>(m_Listener, QueueType::RECEIVED);

        //ig das wird mal error handeling
        if( !pMessages ){
                
        }

        while( limitCounter < REQUEST_LIMIT_PER_SESSION )
        {
            std::optional<Request> msg = pMessages->try_pop();
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
