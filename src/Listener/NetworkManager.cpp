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
#include <thread>
#include <vector>

namespace http{

void NetworkManager::init(){

    SteamNetworkingUtils()->SetGlobalCallback_SteamNetConnectionStatusChanged( sOnConnectionStatusChangedCallback );

    m_pInterface = SteamNetworkingSockets(); 

    m_Core = std::make_unique<NetworkManagerCore>( m_pInterface );

    m_NetworkThread = std::thread ( [this](){ this->run(); } );
}

void NetworkManager::kill(){
    m_running = false;
    m_callbackCV.notify_all();

    if( m_NetworkThread.joinable())
        m_NetworkThread.join();

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

    //literarisch bei jeder methode callen dass engefangen wird zu listenen
    std::lock_guard<std::mutex> _lock(m_ManagerMutex);
    m_Busy=true;
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

void NetworkManager::ConnectionServed( HSteamListenSocket socket, HSteamNetConnection connection ){

    //dummy meVythode die methode aus core in einer queue legt und einen future returned oder sowas
}


const std::vector<HSteamNetConnection>* NetworkManager::getClientList( HSteamListenSocket socket) const{    
    /*
    assert(m_SocketClientsMap.find(socket) != m_SocketClientsMap.end());

    return &(m_SocketClientsMap.at(socket).m_Clients); 
    */
    return nullptr;
}

void NetworkManager::run(){

    std::unique_lock<std::mutex> lock (m_ManagerMutex);

    while ( m_running ){
        m_callbackCV.wait(lock, [this](){
            return m_Busy || !m_running;
        });

        if( !m_running ) break; 

        lock.unlock();

        while( m_Busy ){
            tick();

            if( m_Core->m_SocketClientsMap.empty() && m_FunctionCalls.empty() )
                m_Busy = false;
        }

        lock.lock();
    }
}



void NetworkManager::tick(){
    m_Core->pollFunctionCalls();
    m_Core->pollConnectionChanges();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
}


void NetworkManager::notifySocketCreation( HSteamListenSocket createdSocket, HSteamNetPollGroup pollGroup ){

} 

void NetworkManager::notifySocketDestruction( HSteamListenSocket destroyedSocket ){

}

}
