/**
 * @file DataQueue.cpp
 *
 * This module contains the implementation of the
 * SystemAbstractions::DataQueue class.
 *
 * © 2020 by LiuJ
 */

#include "DataQueue.hpp"

#include <algorithm>
#include <stddef.h>
#include <queue>

namespace {
    
    /**
     * This represents one sequential piece of data being
     *  held in a DataQueue
     */
    struct Element {

        SystemAbstractions::DataQueue::Buffer data;

        size_t consumed = 0;

    };
    
} 

namespace SystemAbstractions {

    struct DataQueue::Impl {

        std::deque< Element > elements;

        size_t totalBytes = 0;

        auto Dequeue(
            size_t numBytesRequested,
            bool returnData,
            bool removeData
        ) -> Buffer {
            Buffer buffer;
            auto nextElement = elements.begin();
            auto bytesLeftFromQueue = std::min(numBytesRequested, totalBytes);
            while (bytesLeftFromQueue > 0) {
                if (
                    (nextElement->consumed == 0) 
                    && (nextElement->data.size() == bytesLeftFromQueue)
                    && buffer.empty()
                ) {
                    if (returnData) {
                        if (removeData) {
                            buffer = std::move(nextElement->data);
                        } else {
                            buffer = nextElement->data;
                        }
                    }
                    if (removeData) {
                        nextElement = elements.erase(nextElement);
                        totalBytes -= bytesLeftFromQueue;
                    }
                    break;
                }
                const auto bytesToConsume = std::min(
                    bytesLeftFromQueue, 
                    nextElement->data.size() - nextElement->consumed
                );
                if (returnData) {
                    (void)buffer.insert(
                        buffer.end(),
                        nextElement->data.begin() + nextElement->consumed,
                        nextElement->data.begin() + nextElement->consumed + bytesToConsume
                    );
                }
                bytesLeftFromQueue -= bytesToConsume;
                if (removeData) {
                    nextElement->consumed += bytesToConsume;
                    totalBytes -= bytesToConsume;
                    if (nextElement->consumed >= nextElement->data.size()) {
                        nextElement = elements.erase(nextElement);
                    }
                } else {
                    if (nextElement->consumed + bytesToConsume >= nextElement->data.size()) {
                        ++nextElement;
                    }
                }
            }
            return buffer;
        }
    };

    DataQueue::~DataQueue() noexcept = default;
    DataQueue::DataQueue(DataQueue&& other) noexcept = default;
    DataQueue& DataQueue::operator=(DataQueue&& other) noexcept = default;

    DataQueue::DataQueue()
        : impl_(new Impl())
    {
    }

    void DataQueue::Enqueue(const Buffer& data) {
        impl_->totalBytes += data.size();
        Element newElement;
        newElement.data = std::move(data);
        impl_->elements.push_back(std::move(newElement));
    }

    void DataQueue::Enqueue(Buffer&& data) {
        impl_->totalBytes += data.size();
        Element newElement;
        newElement.data = std::move(data);
        impl_->elements.push_back(std::move(newElement));
    }

    auto DataQueue::Dequeue(size_t numBytesRequested) -> Buffer {
        return impl_->Dequeue(numBytesRequested, true, true);
    }

    auto DataQueue::Peek(size_t numBytesRequested) -> Buffer {
        return impl_->Dequeue(numBytesRequested, true, false);
    }

    void DataQueue::Drop(size_t numBytesRequested) {
        impl_->Dequeue(numBytesRequested, false, true);
    }

    size_t DataQueue::GetBuffersQueued() const noexcept {
        return impl_->elements.size();
    }

    size_t DataQueue::GetBytesQueued() const noexcept {
        return impl_->totalBytes;
    }

} // namespace SystemAbstractions
