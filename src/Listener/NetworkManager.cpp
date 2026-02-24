#include "http/NetworkManager.h"

#include "Datastrucutres/ThreadSaveQueue.h"
#include "steam/steamnetworkingtypes.h"
#include "http/HTTPinitialization.h"

#include <cassert>
#include <chrono>
#include <cstring>
#include <future>
#include <memory>
#include <mutex>
#include <sys/mman.h>
#include <sys/types.h>
#include <thread>
#include <utility>

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
    notifyFunktionCall();

    return executeFunktion([=](){
        return this->m_Core->createListener(ListenerName);
    });
}

//can return invalid listener
Result<void> NetworkManager::DestroyListener( HListener listener ){
    notifyFunktionCall();

    return executeFunktion([=](){
        return this->m_Core->DestroyListener( listener );
    });
}

//kann Socket initliazition failed returnrn
//kann invald listener returnrn
Result<void> NetworkManager::startListening( HListener listener, u_int16_t port ){
    notifyFunktionCall();

    return executeFunktion([=](){
        return this->m_Core->startListening( listener, port );
    });
}

//can return invalid Listener
Result<void> NetworkManager::stopListening( HListener listener ){
    notifyFunktionCall();

    return executeFunktion([=](){
        return this->m_Core->stopListening( listener );
    });
}

//überlegen result wegen listener fehler der returende werden kann
template<typename T>
ThreadSaveQueue<T>* NetworkManager::getQueue( HListener listener, QueueType queueType ){
    notifyFunktionCall();

    return executeFunktion([=](){
        return this->m_Core->getQueue<T>( listener, queueType );
    });
}

void NetworkManager::ConnectionServed( HSteamListenSocket socket, HSteamNetConnection connection ){
    notifyFunktionCall();

    m_FunctionCalls.push([=](){
        this->m_Core->ConnectionServed( socket, connection );
    });
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
    //Wie machen wir das schön gegen wir die queue als * rein? Oder nehmen wir die methoden doch hier als private rein
    //-->würde keinen sinn wirklich machen wegen testing
    m_Core->pollFunctionCalls( &m_FunctionCalls );
    m_Core->pollConnectionChanges();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
}

void NetworkManager::notifyFunktionCall() {
    std::lock_guard<std::mutex> _lock(m_ManagerMutex);
    m_Busy=true;
    m_callbackCV.notify_one();
}

template<typename Funktion>
auto NetworkManager::executeFunktion(Funktion&& func) -> decltype(func()){
    using returnVal = decltype(func());

    std::promise<returnVal> promisedVal;

    std::future<returnVal> future = promisedVal.get_future();

    m_FunctionCalls.push([func = std::forward<Funktion>(func), promisedVal = std::move(promisedVal)]() mutable {
        promisedVal.set_value(func());
    });
    
    return future.get();
}
}
