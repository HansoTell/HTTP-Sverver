#include "Datastrucutres/ThreadSaveQueue.h"
#include "http/HTTPinitialization.h"
#include "http/NetworkManager.h"

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


}
