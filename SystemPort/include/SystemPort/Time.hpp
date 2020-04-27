#ifndef SYSTEM_ABSTRACTIONS_TIME_HPP
#define SYSTEM_ABSTRACTIONS_TIME_HPP

/**
 * @file Time.hpp
 *
 * This module declares the SystemAbstractions::Time class.
 *
 * © 2020 by LiuJ
 */

#include <memory>
#include <time.h>

namespace SystemAbstractions {

    class Time {
    public:
        ~Time() noexcept;
        Time(const Time&) = delete;
        Time(Time&&) noexcept;
        Time& operator=(const Time&) = delete;
        Time& operator=(Time&&) noexcept;

        Time();

        /**
         * This method returns the amount of time, in second, that
         * has elapsed since this object was created. The time is
         * measured using the system's high-precision perforance counter,
         * typically implemented in hardware (CPU)
         * 
         * @return
         *      The amount of time, in seconds, that 
         *      has elapsed since this object was created is returned.
         */
        double GetTime();

        static struct tm localtime(time_t time = 0);

        static struct tm gmtime(time_t time = 0);

    private:

        struct Impl;

        std::unique_ptr< Impl > impl_;

    };

}

#endif