#include "listener.h"


namespace http{

    Listener::Listener() : m_listening(false), m_running(true){ 
        m_pInterface = NetworkManager::Get().m_pInterface;
        assert(m_pInterface != nullptr);
        
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


        m_Socket = m_pInterface->CreateListenSocketIP(address, numOptions, options);

        if( m_Socket == k_HSteamListenSocket_Invalid){
            auto error = MAKE_ERROR(HTTPErrors::eSocketInitializationFailed, "Failed to initialize Socket");             
            LOG_VERROR(error, "On Port", port);
            return error; 
        }

        NetworkManager::Get().startCallbacksIfNeeded();

        m_pollGroup = m_pInterface->CreatePollGroup();

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
                pollOutMessages();
                pollIncMessages();

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

            int message_Recived = m_pInterface->ReceiveMessagesOnPollGroup(m_pollGroup, &pIncMessage, 1);

            if( message_Recived == 0 )
                break;

            if( message_Recived < 0 ){

                std::lock_guard<std::mutex> _lock (m_Queues->m_ErrorQMutex);

                auto err = MAKE_ERROR(HTTPErrors::ePollGroupHandlerInvalid, "PollGroup Handler invalid on trying to recive Messages");
                LOG_VERROR(err);
                m_Queues->m_ErrorQueue.push(err);

                //Thread pausieren bis server entscheidung getroffen hat wie weiter geht
                m_listening = false;
            }

            Request message;

            message.m_Message.assign((const char*) pIncMessage->m_pData, pIncMessage->m_cbSize);

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
            Request OutMessage;

            {
                std::lock_guard<std::mutex> _lock (m_Queues->m_OutMsgMutex);

                //hoffen das das aus while breacked
                if( m_Queues->m_OutGoingQueues.empty() )
                    break;

                Request& OutMessage = m_Queues->m_OutGoingQueues.front();

                OutMessage = std::move(m_Queues->m_OutGoingQueues.front());

                m_Queues->m_OutGoingQueues.pop();
            }

            const char* msg_c = OutMessage.m_Message.c_str();
            m_pInterface->SendMessageToConnection(OutMessage.m_Connection, &msg_c, (u_int32_t)strlen(msg_c), k_nSteamNetworkingSend_Reliable, nullptr);
        }

    }

    void Listener::DestroySocket(){

        //alles wichtige erst noch handeln oder alle connections einzeln schließen nur demonstration


        //muss ich das socket auf 0 setzten oder so? damit dann nicht falsche dinge passieren
        //muss ich poll group auf null setzten oder so und mus ich checkenb ob false returned wird
        m_pInterface->DestroyPollGroup( m_pollGroup );
        NetworkManager::Get().notifySocketDestruction( m_Socket );
        m_pInterface->CloseListenSocket( m_Socket );

        //uniqer handel oder so für debug ider so wäre schön weil weiß ja niemand was für ein socket jetzt geöffnet oder geschloßen wurde
        LOG_INFO("Destroyed Socket");
    }
}