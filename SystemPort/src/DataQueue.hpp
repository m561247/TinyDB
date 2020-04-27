#ifndef SYSTEM_ABSTRACTIONS_DATA_QUEUE_HPP
#define SYSTEM_ABSTRACTIONS_DATA_QUEUE_HPP

/**
 * @file DataQueue.hpp
 *
 * This module declares the SystemAbstractions::DataQueue class.
 *
 * © 2020 by LiuJ
 */

#include <memory>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

namespace SystemAbstractions {

    /**
     *  This class represent q queue of buffers of data
     */
    class DataQueue {
    public:
        typedef std::vector< uint8_t > Buffer;

    // Lifecycle management
    public:
        ~DataQueue() noexcept;
        DataQueue(const DataQueue&) = delete;
        DataQueue(DataQueue&& other) noexcept;
        DataQueue& operator=(const DataQueue& other) = delete;
        DataQueue& operator=(DataQueue&& other) noexcept;

    public:
        DataQueue();

        /**
         * This method puts a copy of the given data onto the 
         *  end of the queue
         */
        
        void Enqueue(const Buffer& data);

        void Enqueue(Buffer&& data);

        /**
         *  This method tries to remove the given number of bytes from
         *  the queue. Fewer bytes may be returned if there are fewer   
         *  bytes in the queue than request
         */
        Buffer Dequeue(size_t numBytesRequests);

        /**
         *  This method tries to copy the given number of bytes from
         *  the queue. Fewer bytes may be returned if there are fewer
         *  bytes in the queue than requested.
         */
        Buffer Peek(size_t numBytesRequested);

        /**
         *  This method tries to remove the given number of bytes from
         *  the queue. Fewer bytes may be removed if there are fewer
         *  bytes in the queue than requested
         */
        void Drop(size_t numBytesRequested);

        /**
         *  This method returns the number of distinct buffers of data
         *  currently held in the queue.
         * 
         */
        size_t GetBuffersQueued() const noexcept;

        /**
         *  This method returns the number of bytes of data
         *  currently held in the queue
         */
        size_t GetBytesQueued() const noexcept;

    private:

        struct Impl;

        std::unique_ptr< Impl > impl_;

    };

}

#endif /* SYSTEM_ABSTRACTIONS_DATA_QUEUE_HPP */