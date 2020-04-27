/**
 * @file SubprocessPosix.cpp
 *
 * This module contains the POSIX implementation of the
 * SystemAbstractions::Subprocess class.
 *
 * Copyright (c) 2020 by LiuJ
 */

#include "../SubprocessInternal.hpp"

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

namespace {

    // string 转 vector
    std::vector< char > VectorFromString(const std::string& s) {
        std::vector< char > v(s.length() + 1);
        for (size_t i = 0; i < s.length(); ++i) {
            v[i] = s[i];
        }
        return v;
    }

}


namespace SystemAbstractions {

    struct Subprocess::Impl {

        std::thread worker;

        std::function< void() > childExited;

        std::function< void() > childCrashed;

        pid_t child = -1;

        int pipe = -1;

        void (*previousSignalHandler)(int) = nullptr;

        static void SignalHandler(int) {
        }

        void MonitorChild() {
            previousSignalHandler = signal(SIGINT, SignalHandler);
            for(;;) {
                uint8_t token;
                auto amtRead = read(pipe, &token, 1);
                if (amtRead > 0) {
                    childExited();
                    break;
                } else if (
                    (amtRead == 0) 
                    || (
                        (amtRead < 0)
                        && (errno != EINTR)
                    )
                ) {
                    childCrashed();
                    break;
                }
            }
            (void)waitpid(child, NULL, 0);
            (void)signal(SIGINT, previousSignalHandler);
        }

        void JoinChild() {
            if (worker.joinable()) {
                worker.join();
                child = -1;
                (void)close(pipe);
                pipe = -1;
            }
        }
    };

    Subprocess::~Subprocess() noexcept {
        impl_->JoinChild();
        if (impl_->pipe >= 0) {
            uint8_t token = 42;
            (void)write(impl_->pipe, &token, 1);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            (void)close(impl_->pipe);
        }
    }
    Subprocess::Subprocess(Subprocess&&) noexcept = default;
    Subprocess& Subprocess::operator=(Subprocess&&) noexcept = default;

    Subprocess::Subprocess()
        : impl_(new Impl()) 
    {
    }

    unsigned int Subprocess::StartChild(
        std::string program,
        const std::vector< std::string >& args,
        std::function< void() > childExited,
        std::function< void() > childCrashed
    ) {
        impl_->JoinChild();
        impl_->childExited = childExited;
        impl_->childCrashed = childCrashed;
        int pipeEnds[2];
        if (pipe(pipeEnds) < 0) {
            return false;
        }

        std::vector< std::vector< char > > childArgs;
        childArgs.emplace_back(VectorFromString(program));
        childArgs.emplace_back(VectorFromString("child"));
        childArgs.emplace_back(VectorFromString(CppStringPlus::sprintf("%d", pipeEnds[1])));
        for (const auto arg: args) {
            childArgs.emplace_back(VectorFromString(arg));
        }

        impl_->child = fork();
        if (impl_->child == 0) {
            CloseAllFilesExcept(pipeEnds[1]);
            std::vector< char* > argv(childArgs.size() + 1);
            for (size_t i = 0; i < childArgs.size(); ++i) {
                argv[i] = &childArgs[i][0];
            }
            argv[childArgs.size()] = NULL;
            (void)execv(program.c_str(), &argv[0]);
            (void)exit(-1);
        } else if (impl_->child < 0) {
            (void)close(pipeEnds[0]);
            (void)close(pipeEnds[1]);
            return 0;
        }
        impl_->pipe = pipeEnds[0];
        (void)close(pipeEnds[1]);
        impl_->worker = std::thread(&Impl::MonitorChild, impl_.get());
        return (unsigned int)impl_->child;
    }

    unsigned int Subprocess::StartDetached(
        std::string program,
        const std::vector< std::string >& args
    ) {
        int pipeEnds[2];
        if (pipe(pipeEnds) < 0) {
            return 0;
        }
        std::vector< std::vector< char > > childArgs;
        childArgs.push_back(VectorFromString(program));
        for (const auto arg: args) {
            childArgs.push_back(VectorFromString(arg));
        }
        const auto child = fork();
        if (child == 0) {
            CloseAllFilesExcept(pipeEnds[1]);
            (void)setsid();
            const auto grandchild = fork();
            if (grandchild == 0) {
                (void)close(pipeEnds[1]);
                std::vector< char* > argv(childArgs.size() + 1);
                for (size_t i = 0; i < childArgs.size(); ++i) {
                    argv[i] = &childArgs[i][0];
                }
                argv[childArgs.size()] = NULL;
                (void)execv(program.c_str(), &argv[0]);
                (void)exit(-1);
            } else if (grandchild < 0) {
                exit(-1);
            }
            const auto processId = (unsigned int)grandchild;
            (void)write(pipeEnds[1], &processId, sizeof(processId));
            exit(0);
        } else if (child < 0) {
            (void)close(pipeEnds[0]);
            (void)close(pipeEnds[1]);
            return 0;
        }
        (void)close(pipeEnds[1]);
        int childStatus;
        (void)waitpid(child, &childStatus, 0);
        if (WEXITSTATUS(childStatus) != 0) {
            (void)close(pipeEnds[0]);
            return 0;
        }
        unsigned int detachedProcessId;
        const auto readAmount = read(pipeEnds[0], &detachedProcessId, sizeof(detachedProcessId));
        (void)close(pipeEnds[0]);
        if (readAmount == sizeof(detachedProcessId)) {
            return detachedProcessId;
        } else {
            return 0;
        }
    }

    bool Subprocess::ContactParent(std::vector< std::string >& args) {
        if (
            (args.size() >= 2)
            && (args[0] == "child")
        ) {
            int pipeNumber;
            if (sscanf(args[1].c_str(), "%d", &pipeNumber) != 1) {
                return false;
            }
            impl_->pipe = pipeNumber;
            args.erase(args.begin(), args.end() + 2);
            return true;
        }
        return false;
    } 

    unsigned int Subprocess::GetCurrentProcessId() {
        return (unsigned int)getpid();
    }

    void Subprocess::Kill(unsigned int id) {
        (void)kill(id, SIGKILL);
    }

}