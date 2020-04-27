/**
 * @file DiagnosticsContext.cpp
 *
 * This module contains the implementation of the
 * SystemAbstractions::DiagnosticsContext class.
 *
 * Copyright (c) 2020 LiuJ
 */

#include <SystemPort/DiagnosticsContext.hpp>

namespace SystemAbstractions {

    struct DiagnosticsContext::Impl {

        /**
         * This is the sender upon which this class is pushing a context
         * as long as the class instance exists.
         */
        DiagnosticsSender& diagnosticsSender;

        /**
         * This is the constructor.
         * 
         * @param[in] DiagnosticsSender& newDiagnosticSender
         *     This is the sender upon which this class is pushing a context
         *     as long as the class instance exists.
         */
        Impl(DiagnosticsSender& newDiagnosticsSender) 
            : diagnosticsSender(newDiagnosticsSender)
        {
        }
    };

    DiagnosticsContext::DiagnosticsContext(
        DiagnosticsSender& diagnosticsSender,
        const std::string& context
    ) noexcept 
        : impl_(new Impl(diagnosticsSender))
    {
        diagnosticsSender.PushContext(context);
    }

    DiagnosticsContext::~DiagnosticsContext() noexcept {
        impl_->diagnosticsSender.PopContext();
    }
}