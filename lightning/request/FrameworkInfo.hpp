#pragma once
#include <string>
#include <chrono>

namespace lightning
{
    /**
     * @brief Store specific framework data about the request.
     */
    struct FrameworkInfo
    {

        /**
         * @brief The regex that was used to match the resolver to the request.
         */
        std::string matchedRegex;
        
        /**
         * @brief The milliseconds since epoch when the client was accepted. 
         * 
         */
        uint64_t requestArrivalTime;
    };

    using FrameworkInfo = struct FrameworkInfo;
}