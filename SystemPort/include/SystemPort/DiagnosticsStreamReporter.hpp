#ifndef SYSTEM_ABSTACTIONS_DIAGNOSTICS_STREAM_REPORTER_HPP
#define SYSTEM_ABSTACTIONS_DIAGNOSTICS_STREAM_REPORTER_HPP

/**
 * @file DiagnosticsStreamReporter.hpp
 *
 * This module declares the SystemAbstractions::DiagnosticsStreamReporter class.
 *
 * © 2020 by LiuJ
 */

#include "DiagnosticsSender.hpp"

#include <stdio.h>

namespace SystemAbstractions {

    /**
     * This function returns a new diagnostic message delegate which
     * formats and prints all received diagnostic messages to the given
     * log files, according to the time received, the level indicated,
     * and the received message text.
     * 
     * @param[in] output
     *  This is the file to which to print all diagnostic messages
     *  with levels that are under the "Level::WARNING" level informally
     *  defined in the DiagnosticsSender class.
     * 
     */
    DiagnosticsSender::DiagnosticsMessageDelegate DiagnosticsStreamReporter(
        FILE* output,
        FILE* error
    );

}


#endif