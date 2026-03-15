#include "Error/Errorcodes.h"
#include "http/HTTPinitialization.h"
#include "http/listener.h"
#include <sys/types.h>

//2 sachen einerseits porblem wenn init socket nach start listening aufgerufen dann kommen errors
//--> muss auch bei networkmanager was gecleared werden
namespace http {
ListenerCore::ListenerCore( std::shared_ptr<ISteamNetworkinSocketsAdapter> interface, std::function<void(HSteamListenSocket, HSteamNetConnection)>  ConnectionServedCallback ) 
    : m_pInterface(interface), m_ConnectionServedCallback(std::move(ConnectionServedCallback))
{
        assert(m_pInterface != nullptr);
}

ListenerCore::~ListenerCore() {}

Result<SocketHandlers> ListenerCore::initSocket( u_int16_t port ){

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

    return Result<SocketHandlers> ( { m_Socket, m_pollGroup } );
}

void ListenerCore::DestroySocket() {
    m_pInterface->CloseListenSocket( m_Socket );
    m_Socket = k_HSteamListenSocket_Invalid;

    m_pInterface->DestroyPollGroup( m_pollGroup );
    m_pollGroup = k_HSteamNetPollGroup_Invalid;

    m_OutgoingMessages.clear();
    m_RecivedMessegas.clear();
    m_ErrorQueue.clear();
}

Result<bool> ListenerCore::pollOnce(){
    bool polledOutMsg = pollOutMessages();
    auto polledError_or = pollIncMessages();
    if( polledError_or.isErr() )
        return polledError_or;

    bool polledIncMsg = polledError_or.value();

    return polledIncMsg || polledOutMsg;
}

Result<bool> ListenerCore::pollIncMessages() {

    SteamNetworkingMessage_t* pIncMessage = nullptr;

    int message_Recived = m_pInterface->ReceiveMessagesOnPollGroup(m_pollGroup, &pIncMessage, 1);

    if( message_Recived == 0 )
        return false;

    if( message_Recived < 0 ){

        auto err = MAKE_ERROR(HTTPErrors::ePollGroupHandlerInvalid, "PollGroup Handler invalid on trying to recive Messages");
        LOG_VERROR(err);
        m_ErrorQueue.push(err);

        return err;
    }

    Request message;

    message.m_Message.assign((const char*) pIncMessage->m_pData, pIncMessage->m_cbSize);

    m_RecivedMessegas.push(std::move(message));

    pIncMessage->Release();            

    return true;
}

bool ListenerCore::pollOutMessages() {

    std::optional<Request> OutMessage_opt = m_OutgoingMessages.try_pop();        

    if( !OutMessage_opt.has_value() )
        return false;

    Request& OutMessage = OutMessage_opt.value();

    const char* msg_c = OutMessage.m_Message.c_str();
    m_pInterface->SendMessageToConnection(OutMessage.m_Connection, &msg_c, (u_int32_t)strlen(msg_c), k_nSteamNetworkingSend_Reliable, nullptr);

    m_ConnectionServedCallback(m_Socket, OutMessage.m_Connection);

    return true;
}
}
