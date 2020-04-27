/**
 * @file TimeLinux.cpp
 *
 * This module contains the Linux specific part of the
 * implementation of the SystemAbstractions::Time class.
 *
 * Copyright (c) 2020 by LiuJ
 */

#include <SystemPort/Time.hpp>
#include <time.h>

namespace SystemAbstractions {

    struct Time::Impl {

    };

    Time::Time() 
        : impl_(new Impl())
    {
    }

    Time::~Time() noexcept {
    }

    double Time::GetTime() {
        struct timespec tp;
        if (clock_gettime(CLOCK_REALTIME, &tp) != 0) {
            return 0.0;
        }
        return (double)tp.tv_sec + (double)tp.tv_nsec / 1e9;
    }

}