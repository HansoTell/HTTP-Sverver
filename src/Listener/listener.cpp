#include "http/listener.h"

#include "http/NetworkManager.h"
#include "steam/isteamnetworkingsockets.h"

#include <chrono>
#include <cstring>
#include <optional>
#include <utility>


namespace http{
    //Konstruktor for Testing purpuses
    Listener::Listener(ISteamNetworkingSockets* interface) : m_pInterface(interface), m_listening(false), m_running(true) {
        assert(m_pInterface != nullptr);
        
        m_ListenThread = std::thread([this](){ this->listen(); }); 
    }

    Listener::Listener() : m_listening(false), m_running(true), m_pInterface(NetworkManager::Get().m_pInterface){ 
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

    Result<void> Listener::initSocket( u_int16_t port ){

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

        m_pollGroup = m_pInterface->CreatePollGroup();

        if( m_pollGroup == k_HSteamNetPollGroup_Invalid ){
            auto error = MAKE_ERROR(HTTPErrors::ePollingGroupInitializationFailed, "Failed to initialize Polling Group");
            LOG_VERROR(error, "On Port" , port);
            return error; 
        }

        NetworkManager::Get().notifySocketCreation( m_Socket, m_pollGroup );

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
            LOG_DEBUG("Left Listening while Loop");
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

                auto err = MAKE_ERROR(HTTPErrors::ePollGroupHandlerInvalid, "PollGroup Handler invalid on trying to recive Messages");
                LOG_VERROR(err);
                m_ErrorQueue.push(err);

                //Thread pausieren bis server entscheidung getroffen hat wie weiter geht
                m_listening = false;
                break;
            }

            Request message;

            message.m_Message.assign((const char*) pIncMessage->m_pData, pIncMessage->m_cbSize);

            m_RecivedMessegas.push(std::move(message));

            pIncMessage->Release();            

            messages_recived_counter++;
        }
    }

    void Listener::pollOutMessages(){

        int messages_send_counter = 0;
        
        while( messages_send_counter < MAX_MESSAGES_PER_SESSION ){

            std::optional<Request> OutMessage_opt = m_OutgoingMessages.try_pop();        

            if( !OutMessage_opt.has_value() )
                break;

            Request& OutMessage = OutMessage_opt.value();

            const char* msg_c = OutMessage.m_Message.c_str();
            m_pInterface->SendMessageToConnection(OutMessage.m_Connection, &msg_c, (u_int32_t)strlen(msg_c), k_nSteamNetworkingSend_Reliable, nullptr);
        }

    }

    void Listener::DestroySocket(){

        auto connectionList = NetworkManager::Get().getClientList( m_Socket );

        for( auto con : *connectionList ){
            //was machen wir senden deafult response oder senden wir nichts oder denen die noch nichts gesendet haben keine ahnung wird hiuer gehandelt auf jeden

            m_pInterface->CloseConnection(con, 0, nullptr, false);
        }

        m_pInterface->CloseListenSocket( m_Socket );
        m_Socket = k_HSteamListenSocket_Invalid;
        NetworkManager::Get().notifySocketDestruction( m_Socket );

        m_pInterface->DestroyPollGroup( m_pollGroup );
        m_pollGroup = k_HSteamNetPollGroup_Invalid;

        LOG_VINFO("Destroyed Socket");
    }
}
