#include "http/listener.h"

#include "Error/Errorcodes.h"
#include "Logger/Logger.h"
#include "http/HTTPinitialization.h"

#include <chrono>
#include <cstring>
#include <memory>
#include <sys/types.h>

namespace http{
    Listener::Listener( std::unique_ptr<IListenerCore> core ) 
            : m_listening(false), m_running(true), m_Core(std::move(core)){
        
        LOG_INFO("Started Listening Thread");
        m_ListenThread = std::thread([this](){ this->run(); }); 
    }

    Listener::~Listener(){
        m_running = false;
        stopListening();

        if( m_ListenThread.joinable() )
            m_ListenThread.join();
        LOG_INFO("Stopped Listening Thread");

        m_Core.reset(nullptr);
    }

    Result<SocketHandlers> Listener::initSocket ( u_int16_t port ) {
        if( m_listening )
            return MAKE_ERROR(HTTPErrors::eInvalidCall, "Called Init Socket while listening try calling stopListening first");

        return m_Core->initSocket( port );
    }

    Result<void> Listener::startListening(){

        std::lock_guard<std::mutex> _lock (m_ListenMutex);
        m_listening = true;
        m_ListenCV.notify_one();
        
        return {};
    }

    void Listener::stopListening () {
        std::lock_guard<std::mutex> _lock (m_ListenMutex);
        m_listening = false;
        m_ListenCV.notify_one();
        LOG_INFO("Stopped Listening");
    }

    void Listener::run(){
        std::unique_lock<std::mutex> _lock (m_ListenMutex); 
        while ( m_running )
        {
            m_ListenCV.wait(_lock, [this](){
                return !m_running || m_listening;
            });

            _lock.unlock();

            if( !m_running ) 
                break;

            LOG_DEBUG("Started Listening While Loop");
            while( m_listening && m_running ){
                pollMessages();

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            LOG_DEBUG("Left Listening while Loop");

            m_Core->DestroySocket();
        }
    }

    void Listener::pollMessages(){
        bool pollingSuccesfull = true;
        while( pollingSuccesfull && m_listening && m_running )
        {
            auto polledSmthing = m_Core->pollOnce();
            if( polledSmthing.isErr() ){
                m_listening = false;
                break;
            }
                
            pollingSuccesfull = polledSmthing.value();        
        }
    }
}
