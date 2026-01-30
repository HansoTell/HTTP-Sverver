
#include "NetworkManager.h"

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