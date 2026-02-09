
#include "NetworkManager.h"
#include "steam/steamnetworkingtypes.h"
#include <algorithm>
#include <cassert>

#include <chrono>

namespace http{

void NetworkManager::init(){

    SteamNetworkingUtils()->SetGlobalCallback_SteamNetConnectionStatusChanged( OnConnectionStatusChangedCallback );

    m_pInterface = SteamNetworkingSockets(); 

    m_CallBackThread = std::thread ( [this](){ this->pollConnectionChanges(); } );
}

void NetworkManager::kill(){
    m_running = false;
    m_callbackCV.notify_all();

    if( m_CallBackThread.joinable())
        m_CallBackThread.join();
}

void NetworkManager::callbackManager( SteamNetConnectionStatusChangedCallback_t *pInfo ){
    switch ( pInfo->m_info.m_eState ) {
        case  k_ESteamNetworkingConnectionState_None:
        {
            LOG_CRITICAL("Connection Staus Changed NONE");
            break;
        }
        case k_ESteamNetworkingConnectionState_Connecting:
            break;
        case k_ESteamNetworkingConnectionState_Connected:
            break;
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

void NetworkManager::Disconnected( SteamNetConnectionStatusChangedCallback_t *pInfo ){

    const char* pDebugMsg;
    if( pInfo->m_eOldState != k_ESteamNetworkingConnectionState_Connected ){
        assert( pInfo->m_eOldState == k_ESteamNetworkingConnectionState_Connecting );
    }else {
        if( pInfo->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally){
            pDebugMsg = "Problem detected Locally closing Connection to";
        }else {
            pDebugMsg = "Connection Closed by Peer";
        }
        LOG_VCRITICAL(pDebugMsg, pInfo->m_info.m_szConnectionDescription, pInfo->m_info.m_eEndReason, pInfo->m_info.m_addrRemote, pInfo->m_info.m_szEndDebug );
    }

    
    assert( m_SocketClientsMap.find( pInfo->m_info.m_hListenSocket ) != m_SocketClientsMap.end() );

    //Verbindung entfernen
    auto& connections = m_SocketClientsMap.at(pInfo->m_info.m_hListenSocket);
    auto con_it = std::find( connections.begin(), connections.end(), pInfo->m_hConn );
    assert(con_it != connections.end());
    connections.erase(con_it);

    SteamNetworkingSockets()->CloseConnection( pInfo->m_hConn, 0, nullptr, false );

    return;
}

void NetworkManager::startCallbacksIfNeeded() {
    std::lock_guard<std::mutex> lock (m_callbackMutex);
    m_Connections_open=true;
    m_callbackCV.notify_one();
}

void NetworkManager::pollConnectionChanges(){
    std::unique_lock<std::mutex> lock (m_callbackMutex);

    while ( m_running ){
        m_callbackCV.wait(lock, [this](){
            return m_Connections_open || !m_running;
        });

        if( !m_running ) break; 

        lock.unlock();

        while( m_Connections_open ){
            
            m_pInterface->RunCallbacks();

            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            std::lock_guard<std::mutex> lock(m_connection_lock);
            //funktioniert das beim erasen
            if( m_SocketClientsMap.empty() )
                m_Connections_open = false;
        }

        lock.lock();
    }
}

void NetworkManager::notifySocketCreation( HSteamListenSocket createdSocket ){
    std::lock_guard<std::mutex> lock(m_connection_lock);
    m_SocketClientsMap.emplace(createdSocket, std::vector<HSteamNetConnection> (64));
} 

void NetworkManager::notifySocketDestruction( HSteamListenSocket destroyedSocket ){

    std::lock_guard<std::mutex> lock(m_connection_lock);
    m_SocketClientsMap.erase( destroyedSocket );
}

}
