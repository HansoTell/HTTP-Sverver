#include "Datastrucutres/ThreadSaveQueue.h"
#include "http/HTTPinitialization.h"
#include "http/NetworkManager.h"
#include "steam/isteamnetworkingsockets.h"

namespace http{

NetworkManagerCore::NetworkManagerCore( ISteamNetworkingSockets* interface ) : m_pInterface(interface) {
    //was genau muss hier geschehen

}

NetworkManagerCore::~NetworkManagerCore() {

}

HListener NetworkManagerCore::createListener( const char* ListenerName ){

    assert(m_ListenerHandlerIndex < MAXLOGSIZE);
    assert(m_Listeners.find(m_ListenerHandlerIndex) == m_Listeners.end());

    HListener handler = m_ListenerHandlerIndex;

    m_Listeners.emplace(handler, ListenerInfo(std::make_unique<Listener>(m_pInterface)) );
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
    
    //muss socket und pollgroup return
    
    ListenerInfo& info = m_Listeners.at(listener);

    if(auto err = info.m_Listener->startListening( port ); err.isErr() )
        return err;

    info.m_Socket = 1;
    info.m_PollGroup = 2;
    
    return {};
}

Result<void> NetworkManagerCore::stopListening( HListener listener ){

    if(auto err = isValidListenerHandler(listener); err.isErr())
        return err;

    //Frage halt müssen wir socket raus nehmen aus interner liste... eig ja schon implioziert das ja schon
    //und was ist mit diesem einen error den wir ablegen? was wird da eig gecalled
    //finden wir es so gut, dass bei einem einfachem error direkt das socket zerstört wird??
    //notifySocketDestruction problem dass das immer wenn auch implizit das aufgerufen wird--> sollten da eig allgemein eine bessere lösung finden so zu viel implizit
    //
    m_Listeners.at(listener).m_Listener->stopListening();


    return {};
}


template<typename T>
ThreadSaveQueue<T>* NetworkManagerCore::getQueue( HListener listener, QueueType queueType ){
    
    //machen wir ernst result???
    if(auto err = isValidListenerHandler(listener); err.isErr())
        return nullptr;

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

//Muss noch richtig migiruert werden
void NetworkManagerCore::Connecting( SteamNetConnectionStatusChangedCallback_t* pInfo ){
        
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


    //Gucken wie man migiriert
    /*
    assert( m_SocketClientsMap.find( pInfo->m_info.m_hListenSocket ) != m_SocketClientsMap.end() );

    //Verbindung entfernen
    {
        std::lock_guard<std::mutex> _lock (m_connection_lock);
        auto& connections = m_SocketClientsMap.at(pInfo->m_info.m_hListenSocket).m_Clients;
        auto con_it = std::find( connections.begin(), connections.end(), pInfo->m_hConn );
        assert(con_it != connections.end());
        connections.erase(con_it);
    }

    */

    SteamNetworkingSockets()->CloseConnection( pInfo->m_hConn, 0, nullptr, false );

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

    //Schauen wie man migriert zu neuem desing
    /*
    assert( m_SocketClientsMap.find( pInfo->m_info.m_hListenSocket ) != m_SocketClientsMap.end() );

    //Verbindung entfernen
    {
        std::lock_guard<std::mutex> _lock (m_connection_lock);
        auto& connections = m_SocketClientsMap.at(pInfo->m_info.m_hListenSocket).m_Clients;
        auto con_it = std::find( connections.begin(), connections.end(), pInfo->m_hConn );
        assert(con_it != connections.end());
        connections.erase(con_it);
    }

    */

    SteamNetworkingSockets()->CloseConnection( pInfo->m_hConn, 0, nullptr, false );

    return;
}

}
