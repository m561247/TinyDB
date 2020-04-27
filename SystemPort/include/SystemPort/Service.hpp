#ifndef SYSTEM_ABSTRACTION_SERVICE_HPP
#define SYSTEM_ABSTRACTION_SERVICE_HPP

/**
 * @file Service.hpp
 *
 * This module declares the SystemAbstractions::Service class.
 *
 * © 2020 by LiuJ
 */

#include <memory>
#include <string>
#include <vector>

namespace SystemAbstractions {

    class Service
    {
    public:
        ~Service() noexcept;
        Service(const Service&) = delete;
        Service(Service&&) noexcept;
        Service& operator=(const Service&) = delete;
        Service& operator=(Service&&) noexcept;

    public:
        Service();

        int Main();

    protected:

        virtual int Run() = 0;

        virtual void Stop() = 0;

        virtual std::string GetServiceName() const = 0;

    private:

        struct Impl;

        std::unique_ptr< Impl > impl_;
    
    };
    
}

#endif