#include "listener.h"


namespace http{

    Listener::Listener() : m_listening(false), m_running(true){ m_ListenThread = std::thread([this](){ this->listen(); }); }

    Listener::~Listener(){
        m_running = false;
        m_ListenCV.notify_all();

        if( m_ListenThread.joinable() )
            m_ListenThread.join();
    }

    Result<void> Listener::startListening( u_int16_t port ){
        std::lock_guard<std::mutex> _lock (m_ListenMutex);
        if( auto result = initSocket( port ); result.isErr() )
            return result;
        
        m_listening = true;
        m_ListenCV.notify_one();

        return {};
    }

    void Listener::stopListening () {
        std::lock_guard<std::mutex> _lock (m_ListenMutex);
        m_listening = false;
        m_ListenCV.notify_one();
    }

    Result<void> Listener::initSocket( u_int16_t port){

        SteamNetworkingIPAddr address;
        address.Clear();
        address.m_port = port;

        //Set initial Options
        int numOptions = 2;
        SteamNetworkingConfigValue_t options[numOptions];
        options[0].SetInt32(k_ESteamNetworkingConfig_TimeoutInitial, 5000);
        options[1].SetInt32(k_ESteamNetworkingConfig_TimeoutConnected, 5000);


        m_Socket = NetworkManager::Get().m_pInterface->CreateListenSocketIP(address, numOptions, options);

        if( m_Socket == k_HSteamListenSocket_Invalid){
            auto error = MAKE_ERROR(HTTPErrors::eSocketInitializationFailed, "Failed to initialize Socket");             
            LOG_VERROR(error, "On Port", port);
            return error; 
        }

        NetworkManager::Get().startCallbacksIfNeeded();

        m_pollGroup = NetworkManager::Get().m_pInterface->CreatePollGroup();

        if( m_pollGroup == k_HSteamNetPollGroup_Invalid ){
            auto error = MAKE_ERROR(HTTPErrors::ePollingGroupInitializationFailed, "Failed to initialize Polling Group");
            LOG_VERROR(error, "On Port" , port);
            return error; 
        }

        return {};
    }

    void Listener::listen(){
        std::unique_lock<std::mutex> _lock (m_ListenMutex); 
        while ( m_running )
        {
            m_ListenCV.wait(_lock, [this](){
                return !m_running || m_listening;
            });

            _lock.unlock();

            if( !m_running ) 
                break;

            while( m_listening ){

                //poll messegas und son scheiß u know
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            DestroySocket();
        }
    }

    void Listener::DestroySocket(){

        //alles wichtige erst noch handeln oder alle connections einzeln schließen nur demonstration


        //muss ich das socket auf 0 setzten oder so? damit dann nicht falsche dinge passieren
        //muss ich poll group auf null setzten oder so und mus ich checkenb ob false returned wird
        NetworkManager::Get().m_pInterface->DestroyPollGroup( m_pollGroup );
        NetworkManager::Get().notifySocketDestruction( m_Socket );
        NetworkManager::Get().m_pInterface->CloseListenSocket( m_Socket );
    }
}