/**
 * @file SubprocessLinux.cpp
 *
 * This module contains the Linux implementation of the
 * SystemAbstractions::Subprocess class.
 *
 * © 2020 by LiuJ
 */

#include <assert.h>
#include <errno.h>
#include <fstream>
#include <inttypes.h>
#include <limits.h>
#include <map>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <CppStringPlus/CppStringPlus.hpp>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <SystemPort/File.hpp>
#include <SystemPort/Subprocess.hpp>
#include <thread>
#include <unistd.h>
#include <vector>

namespace SystemAbstractions {

    void CloseAllFileExcept(int keepOpen) {
        std::vector< std::string > fds;
        const std::string fdsDir("/proc/self/fd/");
        SystemAbstractions::File::ListDirectory(fdsDir, fds);
        for (const auto& fd: fds) {
            const auto fdNumString = fd.substr(fdsDir.length());
            int fdNum;
            if (
                (sscanf(fdNumString.c_str(), "%d", &fdNum) == 1)
                && (fdNum != keepOpen) 
            ) {
                (void)close(fdNum);
            }
        }
    }

    auto Subprocess::GetProcessList() -> std::vector< ProcessInfo > {

        std::vector< std::string > procs;
        const std::string procDir("/proc/");
        SystemAbstractions::File::ListDirectory(procDir, procs);        
        std::vector< ProcessInfo > processes;
        for (const auto& proc: procs) {
            ProcessInfo process;
            const auto pidString = proc.substr(procDir.length());
            if (sscanf(pidString.c_str(), "%u", &process.id) == 1) {
                const std::string exePath(proc + "/exe");
                std::vector< char > buffer(PATH_MAX + 1);
                if (realpath(exePath.c_str(), &buffer[0]) == NULL) {
                    continue;
                } 
                process.image = std::string(buffer.data());
                processes.push_back(std::move(process));
            }
        }

        std::map< unsigned int, uint16_t > inodesToTcpServerPorts;
        {
            // https://guanjunjian.github.io/2017/11/09/study-8-proc-net-tcp-analysis/
            std::ifstream tcpTable("/proc/net/tcp");
            while(
                !tcpTable.fail()
                && !tcpTable.eof()
            ) {
                std::string line;
                (void)std::getline(tcpTable, line);
                unsigned int slot, localAddress, localPort;
                unsigned int remoteAddress, remotePort;
                unsigned int status, txQueue, rxQueue;
                unsigned int tr, when, retransmit, uid, timeout, inode;
                if (
                    sscanf(
                        line.c_str(),
                        "%u:%X:%X %X:%X %X %X:%X %X:%X %X %u %u %u",
                        slot, localAddress, localPort,
                        remoteAddress, remotePort,
                        status, txQueue, rxQueue,
                        tr, when, retransmit, uid, timeout, inode  
                    ) 
                ) {
                    if (status == 10) {
                        // 监听状态
                        inodesToTcpServerPorts[inode] = localPort;
                    }
                }
            }
        }

        for (auto& process: processes) {
            std::vector< std::string > fds;
            const std::string fdsDir(
                CppStringPlus::sprintf(
                    "/proc/%u/fd/",
                    process.id
                )
            );
            SystemAbstractions::File::ListDirectory(fdsDir, fds);
            for (const auto& fd: fds) {
                std::string target;
                std::vector< char > buffer(64);
                while (target.empty()) {
                    const auto used = readlink(
                        fd.c_str(),
                        buffer.data(),
                        buffer.size()
                    );
                    if (used == buffer.size()) {
                        buffer.resize(buffer.size() * 2);
                    } else if (used > 0) {
                        target.assign(
                            buffer.begin(),
                            buffer.begin() + used
                        );
                    } else {
                        break;
                    }
                }
                unsigned int inode;
                if (
                    sscanf(
                        target.c_str(),
                        "socket:[%u]",
                        &inode
                    ) == 1
                ) {
                    auto inodesToTcpServerPortEntry = inodesToTcpServerPorts.find(inode);
                    if (inodesToTcpServerPortEntry != inodesToTcpServerPorts.end()) {
                        (void)process.tcpServerPorts.insert(inodesToTcpServerPortEntry->second);
                    }
                }
            }   
        }

        return processes;
    }

}