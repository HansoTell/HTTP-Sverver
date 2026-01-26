#include "listener.h"


namespace http{

    Listener::Listener() : m_listening(false), m_running(true){ 
        LOG_INFO("Started Listening Thread");
        m_ListenThread = std::thread([this](){ this->listen(); }); 
    }

    Listener::~Listener(){
        m_running = false;
        m_ListenCV.notify_all();

        if( m_ListenThread.joinable() )
            m_ListenThread.join();
        LOG_INFO("Stopped Listening Thread");
    }

    Result<void> Listener::startListening( u_int16_t port ){
        std::lock_guard<std::mutex> _lock (m_ListenMutex);
        if( auto result = initSocket( port ); result.isErr() )
            return result;
        
        m_listening = true;
        m_ListenCV.notify_one();

        LOG_VINFO("Started Listening on Port", port);

        return {};
    }

    void Listener::stopListening () {
        std::lock_guard<std::mutex> _lock (m_ListenMutex);
        m_listening = false;
        m_ListenCV.notify_one();
        LOG_INFO("Stopped Listening");
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

            LOG_DEBUG("Started Listening While Loop");
            while( m_listening ){
                pollIncMessages();
                pollOutMessages();

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            LOG_DEBUG("Left Listening whiole Loop");
            DestroySocket();
        }
    }


    #define MAX_MESSAGES_PER_SESSION 100
    void Listener::pollIncMessages(){

        int messages_recived_counter = 0;

        while( messages_recived_counter < MAX_MESSAGES_PER_SESSION ){
            SteamNetworkingMessage_t* pIncMessage = nullptr;

            int message_Recived = NetworkManager::Get().m_pInterface->ReceiveMessagesOnPollGroup(m_pollGroup, &pIncMessage, 1);

            if( message_Recived == 0 )
                break;

            if( message_Recived < 0 ){
                //error handeling keine ahnung wie man das im thread machen soll
                //fatal error methode oder error senden an mein thread oder so keine ahnung?? --> heißt ja connection invalid also socket muss eh erstartet werden oder so keine ahnung
                //oder abgebrochen werdens
            }

            std::string message;
            message.assign((const char*)pIncMessage->m_pData, pIncMessage->m_cbSize);

            //Alterbnative vielleicht performanter alle cachen und erst nach while schleife einfügen --> könnte halt verzögerung erhöhen aber mutex entspannen
            {
                std::lock_guard<std::mutex> _lock (m_Queues->m_IncMsgMutex);
                m_Queues->m_IncomingMessages.push(std::move(message));
            }

            pIncMessage->Release();            

            messages_recived_counter++;
        }
    }

    void Listener::pollOutMessages(){
        int messages_send_counter = 0;

        while( messages_send_counter < MAX_MESSAGES_PER_SESSION ){

            //wie entfernen wir messages und so, also das wir keine kopien machen und auch nicht löschen und das es passt?? Also leseb wöäre ja ok aber wie senden wir
            //und wie speichern wir connection das richtige message zu richtigem client kommt... wäre ja schon sinnvoll haha

            //wie geben wir das weiter als tupel der beiden oder als was??? Oder einbinden in den request string?
        }

    }

    void Listener::DestroySocket(){

        //alles wichtige erst noch handeln oder alle connections einzeln schließen nur demonstration


        //muss ich das socket auf 0 setzten oder so? damit dann nicht falsche dinge passieren
        //muss ich poll group auf null setzten oder so und mus ich checkenb ob false returned wird
        NetworkManager::Get().m_pInterface->DestroyPollGroup( m_pollGroup );
        NetworkManager::Get().notifySocketDestruction( m_Socket );
        NetworkManager::Get().m_pInterface->CloseListenSocket( m_Socket );

        //uniqer handel oder so für debug ider so wäre schön weil weiß ja niemand was für ein socket jetzt geöffnet oder geschloßen wurde
        LOG_INFO("Destroyed Socket");
    }
}