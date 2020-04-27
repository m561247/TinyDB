#ifndef  SYSTEM_PORT_CPYPTO_RANDOM_HPP
#define  SYSTEM_PORT_CPYPTO_RANDOM_HPP

/**
 *  
 * @file CryptoRandom.hpp
 *
 * This module declares the SystemAbstractions::CryptoRandom class.
 *
 * © 2020 by LiuJ
 */

#include <memory>
#include <stddef.h>

namespace SystemAbstractions {

    /**
     * This class represents a generator of strong entropy random numbers.
     */
    class CryptoRandom {
    
    public:
        ~CryptoRandom() noexcept;
        CryptoRandom(const CryptoRandom&) = delete;
        CryptoRandom(CryptoRandom&&) noexcept = delete;
        CryptoRandom& operator=(const CryptoRandom&) = delete;
        CryptoRandom& operator=(CryptoRandom&&) noexcept = delete;
        CryptoRandom();

        /**
         *  This method generates strong antropy random numbers and stores
         *  them into the given buffer.
         * 
         *  @param[in] buffer
         *      This points to a buffer in which to store the numbers.
         * 
         *  @param[in] length
         *      This is the length of the buffer, in bytes.
         */
        void Generate(void* buffer, size_t length);

    private:

        struct Impl;

        std::unique_ptr< Impl > impl_;

    };

}

#endif