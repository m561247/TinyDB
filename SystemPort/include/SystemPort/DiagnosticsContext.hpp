#ifndef SYSTEM_ABSTRACTIONS_DIAGNOSTICS_CONTEXT_HPP
#define SYSTEM_ABSTRACTIONS_DIAGNOSTICS_CONTEXT_HPP

/**
 * @file DiagnosticsContext.hpp
 *
 * This module declares the SystemAbstractions::DiagnosticsContext class.
 *
 * © 2020 by LiuJ
 */

#include "DiagnosticsSender.hpp"

#include <string>

namespace SystemAbstractions {

    class DiagnosticsContext {
    public:
        ~DiagnosticsContext() noexcept;
        DiagnosticsContext(const DiagnosticsContext&) = delete;
        DiagnosticsContext(DiagnosticsContext&&) noexcept = delete;
        DiagnosticsContext& operator=(const DiagnosticsContext&) = delete;
        DiagnosticsContext& operator=(DiagnosticsContext&&) noexcept = delete;

        DiagnosticsContext(
            DiagnosticsSender& diagnosticsSender,
            const std::string& context
        ) noexcept;

    private:

        struct Impl;

        std::unique_ptr< Impl > impl_;
    };

}

#endif /* SYSTEM_ABSTRACTIONS_DIAGNOSTICS_CONTEXT_HPP */