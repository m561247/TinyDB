#ifndef SYSTEM_ABSTRACTIONS_DIAGNOSTICS_SENDER_HPP
#define SYSTEM_ABSTRACTIONS_DIAGNOSTICS_SENDER_HPP

/**
 * @file DiagnosticsSender.hpp
 *
 * This module declares the SystemAbstractions::DiagnosticsSender class.
 *
 * © 2020 by LiuJ
 */

#include <functional>
#include <memory>
#include <stdarg.h>
#include <stddef.h>
#include <string>

namespace SystemAbstractions {

    /**
     * This represents an object that sends diagnostic information
     * to other objects.
     */
    class DiagnosticsSender {

    public:

        /**
         *  These are informal level settings for common types
         *  of message such as warnings and errors.
         */
        enum Levels : size_t {
            WARNING = 5,
            ERROR = 10
        };

        /**
         * This is the type of function used to unsubscribe, or
         * remove a previously-formed subscription.
         */
        typedef std::function< void() > UnsubscribeDelegate;

        /**
         * This is the type of function given when subscribing
         *  to diagnostic messages, and called to deliver
         *  any diagnostic messages published while the subscription
         * 
         * @param[in] senderName
         *  This identifies the origin of the diagnostic information.
         * 
         * @param[in] level
         *  This is used to filter out less-important information.
         *  The higher the level, the more important the information is.
         * 
         * @param[in] message
         *  This is the content of the message.
         *  诊断消息委托函数
         * 
         */
        typedef std::function<
            void(
                std::string senderName,
                size_t level,
                std::string mes
            )
        > DiagnosticsMessageDelegate;

        // Lifecycle Management
        ~DiagnosticsSender() noexcept;
        DiagnosticsSender(const DiagnosticsSender&) = delete;
        DiagnosticsSender(DiagnosticsSender&&) noexcept;
        DiagnosticsSender& operator=(const DiagnosticsSender&) = delete;
        DiagnosticsSender& operator=(DiagnosticsSender&&) noexcept;

        // 阻止不应该允许的经过转换构造函数进行的隐式转换发生
        explicit DiagnosticsSender(std::string name);

        /**
         *  This method forms a new subscription to diagnostic
         *  messages published by sender.
         * 
         *  @param[in] delegate
         *     This is the fuction to call to deliver messages to this subscriber
         * 
         *  @param[in] minLevel
         *     This is the minimun level of message that this subscriber 
         *      desires to receive.
         * 
         *  @return
         *      A function is returned which may be called
         *      to terminate the subscription
         * 
         *  minLevel 缺省值 0
         */
        UnsubscribeDelegate SubscribeToDiagnostics(
            DiagnosticsMessageDelegate delegate,
            size_t minLevel = 0
        );


        /**
         *  This method returns a function which can be used to subscribe
         *  the sender to diagnostic message published by another sender
         *  In orrder to chain them together
         * 
         *  @return
         *      A function is returned which can be used to subscribe 
         *      the sender to diagnostic messages published by another sender,
         *      in order to chain them together.
         */
        DiagnosticsMessageDelegate Chain() const;

        /**
         *  This method returns the lowest of all the minimum desired 
         *  message levels for all current subscribers.
         */
        size_t GetMinLevel() const;

        /**
         *  This method publishes a static diagnostic message.
         * 
         *  @param[in] level
         *      This is used to filter out less-important information.
         *      The level is higher the more important the information is.
         * 
         *  @param[in] message
         *      This is the content of the message.
         */
        void SendDiagnosticInformationString(size_t level, std::string message) const;

        /**
         * This method publishes a diagnostic message formatted
         * according to the rules and capabilities of the C standard library
         * function "sprintf"
         * 
         * @param[in] level
         *  This is used to filter out less-important information.
         *  The level is higher the more important the information if
         * 
         * @param[in] format
         *  This is the formatting string to use as a guide to build
         *  the message.
         * 
         */
        void SendDiagnosticInformationFormatted(size_t level, const char* format, ...) const;

        /**
         *  This method adds the given string onto to the top of the contextural
         *  information stack.
         * 
         */
        void PushContext(std::string context);

        /**
         *  This method removes the top string off of the contextual    
         *  information stack.
         */
        void PopContext();

    private:

        struct Impl;

        std::shared_ptr< Impl > impl_;

    };

}

#endif