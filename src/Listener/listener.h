#pragma once

#include <steam/steamnetworkingsockets.h>
#include <stdint.h>
#include <chrono>
#include <thread>

#include "NetworkManager.h"
#include "../Server/HTTPinitialization.h"
#include "../Error/Errorcodes.h"

namespace http{

class Listener {
public:
    Listener();
    Listener(const Listener& other) = delete;
    Listener(Listener&& other) = default;
    ~Listener();
public:
    //Soll alle clients verwalten und so und anfragen akzeptieren messages an die messager queue übergeben
    //Wie bekommen wir message queue??
    void startListening( uint16 port );
    void stopListening();
private:
    Result<void> init();
    void listen();
private:
    HSteamListenSocket m_Socket;
    HSteamNetPollGroup m_pollGroup;

    //Threading stuff
    std::atomic<bool> m_running;
    std::atomic<bool> m_listening = false;
    std::atomic<u_int16_t> m_port;
    std::mutex m_ListenMutex;
    std::condition_variable m_ListenCV;
};

}

