#include "Datastrucutres/ThreadSaveQueue.h"
#include "Error/Errorcodes.h"
#include "http/HTTPinitialization.h"
#include "http/NetworkManager.h"
#include "steam/isteamnetworkingsockets.h"
#include "steam/steamnetworkingtypes.h"
#include <algorithm>
#include <cassert>
#include <sys/types.h>

namespace http{

NetworkManagerCore::NetworkManagerCore( ISteamNetworkingSockets* interface ) : m_pInterface(interface), m_ListenerHandlerIndex(1){}

NetworkManagerCore::~NetworkManagerCore() {}

HListener NetworkManagerCore::createListener( const char* ListenerName ){

    assert(m_Listeners.find(m_ListenerHandlerIndex) == m_Listeners.end());

    HListener handler = m_ListenerHandlerIndex;

    m_Listeners.emplace(handler, ListenerInfo(std::make_unique<Listener>(m_pInterface, NetworkManager::sConnectionServedCallback)) );
    if( ListenerName )
        strncpy(m_Listeners.at(handler).ListenerName, ListenerName, 512);

    m_ListenerHandlerIndex++;

    return handler;
}

Result<void> NetworkManagerCore::DestroyListener( HListener listener ){

    if(auto err = isValidListenerHandler(listener); err.isErr())
        return err;

    m_Listeners.at(listener).m_Listener.reset(nullptr);
    m_Listeners.erase(listener);

    return {};
}

Result<void> NetworkManagerCore::startListening( HListener listener, u_int16_t port ){

    if(auto err = isValidListenerHandler(listener); err.isErr())
        return err;
    
    ListenerInfo& info = m_Listeners.at(listener);

    if( info.m_Socket != k_HSteamListenSocket_Invalid || info.m_Socket != k_HSteamNetPollGroup_Invalid )
        return MAKE_ERROR(HTTPErrors::eInvalidSocket, "Already Listening on this Scoket -> Socket/Pollgroup Valid" );

    auto result = info.m_Listener->initSocket( port );
    if( result.isErr() )
        return Result<void>(result.error());
    
    auto& SocketHandlers = result.value();

    info.m_Socket = SocketHandlers.m_Socket;

    m_SocketClientsMap.emplace(info.m_Socket, SocketInfo(info.m_PollGroup) );

    info.m_Listener->startListening(); 
    
    LOG_VINFO("Started Listening on Port", port);

    return {};
}

Result<void> NetworkManagerCore::stopListening( HListener listener ){

    if(auto err = isValidListenerHandler(listener); err.isErr())
        return err;

    ListenerInfo& info = m_Listeners.at(listener);


    if( info.m_Socket == k_HSteamListenSocket_Invalid )
        return MAKE_ERROR(HTTPErrors::eInvalidSocket, "Not currently Listening (Double Call?)");

    info.m_Listener->stopListening();

    assert(m_SocketClientsMap.find(info.m_Socket) != m_SocketClientsMap.end());

    auto& allConnections = m_SocketClientsMap.at(info.m_Socket).m_AllConnections;

    for( auto& conn : allConnections ){
        if(  !conn.isServed ){}
        //Könnten auch sowas machen wie bei der inc queue alles einkommende zu blocken und dann noch alle deafult responses rein legen
        //damit das thred save abläuft wäre so eine idee
            //info.m_Listener->sendDeafultResponse();
        m_pInterface->CloseConnection(conn.m_connection, 1, nullptr, false);
    }

    m_SocketClientsMap.erase(info.m_Socket);

    info.m_Socket = k_HSteamListenSocket_Invalid;


    info.m_Listener->stopListening();

    LOG_VINFO("Destroyed Socket on Listener", info.ListenerName);

    return {};
}


template<typename T>
Result<ThreadSaveQueue<T>*> NetworkManagerCore::getQueue( HListener listener, QueueType queueType ){
    
    if(auto err = isValidListenerHandler(listener); err.isErr())
        return err;

    auto& pListener = m_Listeners.at(listener); 
    switch ( queueType ) 
    {
        case ERROR:
            return &(pListener.m_Listener->m_ErrorQueue);
        case RECEIVED:
            return &(pListener.m_Listener->m_RecivedMessegas);
        case OUTGOING:
            return &(pListener.m_Listener->m_OutgoingMessages);
    }
}

void NetworkManagerCore::ConnectionServed( HSteamListenSocket socket, HSteamNetConnection connection ){
    assert(m_SocketClientsMap.find(socket) != m_SocketClientsMap.end());

    auto& allClients = m_SocketClientsMap.at(socket).m_AllConnections;

    auto it = std::find_if(allClients.begin(), allClients.end(), [&connection](const Connections& con){
        return con.m_connection == connection;
    }); 

    assert(it != allClients.end());

    it->isServed = true;
}

void NetworkManagerCore::callbackManager( SteamNetConnectionStatusChangedCallback_t* pInfo ){

    switch ( pInfo->m_info.m_eState ) {
        case  k_ESteamNetworkingConnectionState_None:
        {
            LOG_CRITICAL("Connection Staus Changed NONE");
            break;
        }
        case k_ESteamNetworkingConnectionState_Connecting:
        {
            Connecting( pInfo );
            break;
        }
        case k_ESteamNetworkingConnectionState_Connected:
        {
            //No need to add additional things
            break;
        }
        case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
        {
            Disconnected( pInfo );
            break;
        }
        case k_ESteamNetworkingConnectionState_ClosedByPeer:
        {
            Disconnected( pInfo );
            break;
        }
        default:
        {
            LOG_CRITICAL("Undefined Status changed Callback.")  ;
            break;
        }
    }
}

void NetworkManagerCore::Connecting( SteamNetConnectionStatusChangedCallback_t* pInfo ){
    LOG_VINFO("Trying to connect from", pInfo->m_info.m_szConnectionDescription);

    if( m_pInterface->AcceptConnection(pInfo->m_hConn) != k_EResultOK ){
        m_pInterface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
        LOG_VINFO("Couldnt Accept Connection from", pInfo->m_info.m_szConnectionDescription);
        return;
    }

    assert(m_SocketClientsMap.find(pInfo->m_info.m_hListenSocket) != m_SocketClientsMap.end());
    auto& socketInfo = m_SocketClientsMap.at(pInfo->m_info.m_hListenSocket); 
    auto& pollGroup = socketInfo.m_PollGroup;

    if( !m_pInterface->SetConnectionPollGroup(pInfo->m_hConn, pollGroup) ){
        m_pInterface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
        LOG_CRITICAL("Failed to assign PollGroup");
        return;
    }

    socketInfo.m_AllConnections.push_back({ pInfo->m_hConn });    

    LOG_INFO("Connection Accepted!");
    return;
}

void NetworkManagerCore::Disconnected( SteamNetConnectionStatusChangedCallback_t* pInfo ){

    const char* pDebugMsg;
    if( pInfo->m_eOldState != k_ESteamNetworkingConnectionState_Connected ){
        assert( pInfo->m_eOldState == k_ESteamNetworkingConnectionState_Connecting );
    }else {
        if( pInfo->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally){
            pDebugMsg = "Problem detected Locally closing Connection to";
        }else {
            pDebugMsg = "Connection Closed by Peer";
        }
        //kann ip addresse net benutzenm weil net loggable gleiche mit debug discription :( mal gucken ginge sicher mit printf oder so
        LOG_VCRITICAL(pDebugMsg, pInfo->m_info.m_szConnectionDescription, pInfo->m_info.m_eEndReason );
    }

    assert( m_SocketClientsMap.find( pInfo->m_info.m_hListenSocket ) != m_SocketClientsMap.end() );

    auto& connections = m_SocketClientsMap.at(pInfo->m_info.m_hListenSocket).m_AllConnections;
    auto con_it = std::find_if( connections.begin(), connections.end(), [pInfo](const Connections& con){
        return pInfo->m_hConn == con.m_connection;
    });
    assert(con_it != connections.end());
    connections.erase(con_it);
    
    SteamNetworkingSockets()->CloseConnection( pInfo->m_hConn, 0, nullptr, false );

    return;
}

void NetworkManagerCore::pollConnectionChanges(){
    m_pInterface->RunCallbacks();
}

#define MAX_FUNCTIONS_PER_SESSION 20 
void NetworkManagerCore::pollFunctionCalls( ThreadSaveQueue<std::function<void()>>* functionQueue ){
    u_int16_t counter = 0;
    while( !functionQueue->empty() || counter < MAX_FUNCTIONS_PER_SESSION ) {
        auto funk_opt = functionQueue->try_pop();
        if( !funk_opt.has_value() )
            break;
        auto funk = funk_opt.value();
        funk();

        counter++;
    }
}
}
