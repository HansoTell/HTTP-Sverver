#pragma once

#include <queue>
#include <string>
#include <mutex>

namespace http {

    //müssne mutex geschützt sein, weil mehrere threads werden mesages einfügen und entfernen können also shcützen
struct MessageQueues{

    std::queue<std::string> m_IncomingMessages;
    std::mutex m_IncMsgMutex;

    std::queue<std::string> m_OutGoingQueues;
    std::mutex m_OutMsgMutex;

};


}