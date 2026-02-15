#include "http/NetworkManager.h"

#include "Datastrucutres/ThreadSaveQueue.h"
#include "Error/Errorcodes.h"
#include "Logger/Logger.h"
#include "steam/steamclientpublic.h"
#include "steam/steamnetworkingtypes.h"
#include "http/HTTPinitialization.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <memory>
#include <mutex>
#include <sys/types.h>
#include <vector>

namespace http{

void NetworkManager::init(){

    SteamNetworkingUtils()->SetGlobalCallback_SteamNetConnectionStatusChanged( OnConnectionStatusChangedCallback );

    m_pInterface = SteamNetworkingSockets(); 

    m_ListenerHandlerIndex = 1;

    m_CallBackThread = std::thread ( [this](){ this->pollConnectionChanges(); } );
}

void NetworkManager::kill(){
    m_running = false;
    m_callbackCV.notify_all();

    if( m_CallBackThread.joinable())
        m_CallBackThread.join();
}

HListener NetworkManager::createListener() {

    assert(m_ListenerHandlerIndex < MAXLOGSIZE);
    assert(m_Listeners.find(m_ListenerHandlerIndex) == m_Listeners.end());

    HListener handler = m_ListenerHandlerIndex;
    m_Listeners.emplace(handler, std::make_unique<Listener>());
    m_ListenerHandlerIndex++;

    return handler;
}

//can return invalid listener
Result<void> NetworkManager::DestroyListener( HListener listener ){

    if(auto err = isValidListenerHandler(listener); err.isErr())
        return err;

    m_Listeners.at(listener).reset(nullptr);
    m_Listeners.erase(listener);

    return {};
}

Result<void> NetworkManager::startListening( HListener listener, u_int16_t port){

    if(auto err = isValidListenerHandler(listener); err.isErr())
        return err;

    //TODO: Wie machen mit namen des listener hier speichern oder gar nicht speichern oder was 
    m_Listeners.at(listener)->startListening( port, "");


    std::lock_guard<std::mutex> _lock (m_connection_lock);
    m_Connections_open=true;
    m_callbackCV.notify_one();
                                             
    return {};
}

//can return invalid Listener
Result<void> NetworkManager::stopListening( HListener listener ){

    if(auto err = isValidListenerHandler(listener); err.isErr())
        return err;

    //Frage halt müssen wir socket raus nehmen aus interner liste... eig ja schon implioziert das ja schon
    //und was ist mit diesem einen error den wir ablegen? was wird da eig gecalled
    //finden wir es so gut, dass bei einem einfachem error direkt das socket zerstört wird??
    //notifySocketDestruction problem dass das immer wenn auch implizit das aufgerufen wird--> sollten da eig allgemein eine bessere lösung finden so zu viel implizit
    //
    m_Listeners.at(listener)->stopListening();


    return {};
}

//Auch nicht wirklich schön mit Template muster...
template<typename T>
ThreadSaveQueue<T>* NetworkManager::getQueue( HListener listener, QueueType queueType ){
    
    //machen wir ernst result???
    if(auto err = isValidListenerHandler(listener); err.isErr())
        return nullptr;

    auto& pListener = m_Listeners.at(listener); 
    switch ( queueType ) 
    {
        case ERROR:
            return &(pListener->m_ErrorQueue);
        case RECEIVED:
            return &(pListener->m_RecivedMessegas);
        case OUTGOING:
            return &(pListener->m_OutgoingMessages);
    }
}


const std::vector<HSteamNetConnection>* NetworkManager::getClientList( HSteamListenSocket socket) const{    
    assert(m_SocketClientsMap.find(socket) != m_SocketClientsMap.end());

    return &(m_SocketClientsMap.at(socket).m_Clients); 
}

void NetworkManager::callbackManager( SteamNetConnectionStatusChangedCallback_t *pInfo ){
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
        //kann ip addresse net benutzenm weil net loggable gleiche mit debug discription :( mal gucken ginge sicher mit printf oder so
        LOG_VCRITICAL(pDebugMsg, pInfo->m_info.m_szConnectionDescription, pInfo->m_info.m_eEndReason );
    }

    
    assert( m_SocketClientsMap.find( pInfo->m_info.m_hListenSocket ) != m_SocketClientsMap.end() );

    //Verbindung entfernen
    {
        std::lock_guard<std::mutex> _lock (m_connection_lock);
        auto& connections = m_SocketClientsMap.at(pInfo->m_info.m_hListenSocket).m_Clients;
        auto con_it = std::find( connections.begin(), connections.end(), pInfo->m_hConn );
        assert(con_it != connections.end());
        connections.erase(con_it);
    }

    SteamNetworkingSockets()->CloseConnection( pInfo->m_hConn, 0, nullptr, false );

    return;
}

void NetworkManager::Connecting( SteamNetConnectionStatusChangedCallback_t* pInfo ){
    LOG_VINFO("Trying to connect from", pInfo->m_info.m_szConnectionDescription);
    
    if( m_pInterface->AcceptConnection(pInfo->m_hConn) != k_EResultOK ){
        m_pInterface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
        LOG_VINFO("Couldnt Accept Connection from", pInfo->m_info.m_szConnectionDescription);
        return;
    }
    
    {
        std::lock_guard<std::mutex> _lock (m_connection_lock);
        assert(m_SocketClientsMap.find(pInfo->m_info.m_hListenSocket) != m_SocketClientsMap.end());
        auto& socketInfo = m_SocketClientsMap.at(pInfo->m_info.m_hListenSocket); 
        auto pollGroup = socketInfo.m_PollGroup;

        if( !m_pInterface->SetConnectionPollGroup(pInfo->m_hConn, pollGroup) ){
            m_pInterface->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
            LOG_CRITICAL("Failed to assign PollGroup");
            return;
        }

        socketInfo.m_Clients.emplace_back(pInfo->m_hConn);
    }

    LOG_INFO("Connection Accepted!");
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

Result<void> NetworkManager::isValidListenerHandler( HListener listenerHandler) const {
    if( m_Listeners.find(listenerHandler) == m_Listeners.end() ){
        auto err = MAKE_ERROR(HTTPErrors::eInvalidListener, "Invalid listener number");
        LOG_VERROR(err);
        return err;
    }
    
    return {};
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
