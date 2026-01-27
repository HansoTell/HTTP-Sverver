#pragma once

namespace http {

    enum HTTPErrors {
        bsp = 0, eSocketInitializationFailed, ePollingGroupInitializationFailed, ePollGroupHandlerInvalid
    };
}