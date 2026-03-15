#include "http/NetworkManager.h"

#include "Datastrucutres/ThreadSaveQueue.h"
#include "Error/Error.h"
#include "Error/Errorcodes.h"
#include "steam/steamnetworkingtypes.h"
#include "http/HTTPinitialization.h"

#include <cassert>
#include <chrono>
#include <cstring>
#include <memory>
#include <mutex>
#include <optional>
#include <sys/mman.h>
#include <sys/types.h>
#include <thread>
#include <utility>

namespace http{

Result<void> NetworkManager::init( std::unique_ptr<INetworkManagerCore> core, std::shared_ptr<ISteamNetworkinSocketsAdapter> pInterface ){

    if( m_initialized )
        return MAKE_ERROR(HTTPErrors::eInvalidCall, "NetworkManager Already initialized need to call Kill first");

    m_pInterface = pInterface;
    
    m_Core = std::move(core);

    m_pInterface->SetGlobalCallback_SteamNetConnectionStatusChanged( sOnConnectionStatusChangedCallback );

    m_running = true;

    m_initialized = true;

    m_NetworkThread = std::thread ( [this](){ this->run(); } );

    return {};
}

void NetworkManager::kill(){
    m_running = false;
    m_callbackCV.notify_all();

    if( m_NetworkThread.joinable())
        m_NetworkThread.join();

    m_Core.reset(nullptr);
    m_pInterface = nullptr;
    m_initialized = false;
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

//can return invalid Listener
Result<std::optional<Request>> NetworkManager::try_PoPReceivedMessageQueue( HListener listener ){
    notifyFunktionCall();

    return executeFunktion([=](){
        return this->m_Core->try_PoPReceivedMessageQueue( listener );
    });
}

//kan return invalid Listener, InvalidCall, InvalidConnection
Result<void> NetworkManager::push_OutgoingMessageQueue( HListener listener, Request message ){
    notifyFunktionCall();

    return executeFunktion([=](){
        return this->m_Core->push_OutgoingMessageQueue( listener, message );
    });
}

//can return invalid listener
Result<std::optional<Error::ErrorValue<HTTPErrors>>> NetworkManager::try_PoPErrorQueue( HListener listener ){
    notifyFunktionCall();

    return executeFunktion([=](){
        return this->m_Core->try_PoPErrorQueue( listener );
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

            if( m_Core->isSocketClientsMapEmpty() && m_FunctionCalls.empty() )
                m_Busy = false;
        }

        lock.lock();
    }
}

void NetworkManager::tick(){
    m_Core->pollFunctionCalls( &m_FunctionCalls );
    m_Core->pollConnectionChanges();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
}

void NetworkManager::notifyFunktionCall() {
    std::lock_guard<std::mutex> _lock(m_ManagerMutex);
    m_Busy=true;
    m_callbackCV.notify_one();
}
}
