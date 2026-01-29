#pragma once

#include <queue>
#include <string>
#include <mutex>

#include "../Error/Error.h"

namespace http {

    //müssen alles an queues oder zentrale wartende queue?? auf länge überprüfen, damit nicht zu viele warten und dann vielleciht anfragen blckiert werden
    //oder nur cpu thread pool queueu ob da viel wartet?? oder auch andere ernst keine ahnung glaube aber das was da steht
    //länge von queues auch loggen
    //und mergen wir inc message queue mit aiting api queue -> unbearbeiteteNachrichtenQueue --> wie könnte man zwischen unfinished und api waiting unterscheiden
    //zudem out queue auch toter name irgendwie DoneRequests oder answers oder so so toter name
    //zudem nicht shcön das alle queues so zersprengt sind --> könnten auch diese queues einfach im listener public speichern oder so und dann error queue als referenz oder so
    //dann gibt es wenigstens klare ownerships
struct Request{
    u_int32_t m_Connection;
    std::string m_Message;

    Request() = default;
    Request(const Request& other) = default;
    Request& operator=(const Request& other) { if(this != &other) { m_Connection=other.m_Connection; m_Message = other.m_Message; } return *this; }
    Request(Request&& other) : m_Connection(other.m_Connection), m_Message(std::move(other.m_Message)) { other.m_Connection = 0; other.m_Message=""; }
    Request& operator=(Request&& other) { if(this != &other) { m_Connection=other.m_Connection; m_Message = std::move(other.m_Message); } return *this;  }
    ~Request() = default;
};

struct MessageQueues{

    std::queue<Request> m_IncomingMessages;
    std::mutex m_IncMsgMutex;

    std::queue<Request> m_OutGoingQueues;
    std::mutex m_OutMsgMutex;

    //kann invalid poll group error on listener enthalten
    std::queue<Error::ErrorValue<HTTPErrors>> m_ErrorQueue;
    std::mutex m_ErrorQMutex;

};
}