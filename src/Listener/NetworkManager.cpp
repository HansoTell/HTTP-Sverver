#include "http/NetworkManager.h"

#include "Datastrucutres/ThreadSaveQueue.h"
#include "steam/steamnetworkingtypes.h"
#include "http/HTTPinitialization.h"

#include <cassert>
#include <chrono>
#include <cstring>
#include <memory>
#include <mutex>
#include <sys/types.h>
#include <vector>

namespace http{

void NetworkManager::init(){

    //Zu lambda konvertieren?
    SteamNetworkingUtils()->SetGlobalCallback_SteamNetConnectionStatusChanged( OnConnectionStatusChangedCallback );

    m_pInterface = SteamNetworkingSockets(); 

    m_Core = std::make_unique<NetworkManagerCore>( m_pInterface );

    m_ListenerHandlerIndex = 1;

    m_CallBackThread = std::thread ( [this](){ this->pollConnectionChanges(); } );
}

void NetworkManager::kill(){
    m_running = false;
    m_callbackCV.notify_all();

    if( m_CallBackThread.joinable())
        m_CallBackThread.join();

    //richtige Stelle?
    m_Core.reset(nullptr);
}

HListener NetworkManager::createListener( const char* ListenerName ) {
    //dummy meVythode die methode aus core in einer queue legt und einen future returned oder sowas

    return 1;
}

//can return invalid listener
Result<void> NetworkManager::DestroyListener( HListener listener ){
    //dummy meVythode die methode aus core in einer queue legt und einen future returned oder sowas
    return {};
}

//kann Socket initliazition failed returnrn
//kann invald listener returnrn
Result<void> NetworkManager::startListening( HListener listener, u_int16_t port){

    //dummy meVythode die methode aus core in einer queue legt und einen future returned oder sowas

    std::lock_guard<std::mutex> _lock (m_connection_lock);
    m_Connections_open=true;
    m_callbackCV.notify_one();
                                             
    return {};
}

//can return invalid Listener
Result<void> NetworkManager::stopListening( HListener listener ){

    //dummy meVythode die methode aus core in einer queue legt und einen future returned oder sowas

    return {};
}

template<typename T>
ThreadSaveQueue<T>* NetworkManager::getQueue( HListener listener, QueueType queueType ){
    //dummy meVythode die methode aus core in einer queue legt und einen future returned oder sowas

    return nullptr;
}


const std::vector<HSteamNetConnection>* NetworkManager::getClientList( HSteamListenSocket socket) const{    
    assert(m_SocketClientsMap.find(socket) != m_SocketClientsMap.end());

    return &(m_SocketClientsMap.at(socket).m_Clients); 
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

void NetworkManager::pollFunctionCalls(){
    //wie integriert man?

}


void NetworkManager::notifySocketCreation( HSteamListenSocket createdSocket, HSteamNetPollGroup pollGroup ){
    std::lock_guard<std::mutex> lock(m_connection_lock);

    m_SocketClientsMap.emplace(createdSocket, pollGroup);
} 

void NetworkManager::notifySocketDestruction( HSteamListenSocket destroyedSocket ){

    std::lock_guard<std::mutex> lock(m_connection_lock);
    m_SocketClientsMap.erase( destroyedSocket );
}

}
