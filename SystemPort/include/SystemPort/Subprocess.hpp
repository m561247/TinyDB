#ifndef SYSTEM_PORT_SUBPROCESS_HPP
#define SYSTEM_PORT_SUBPROCESS_HPP

/**
 * @file Subprocess.hpp
 *
 * This module declares the SystemAbstractions::Subprocess class.
 *
 * © 2020 by LiuJ
 */

#include <functional>
#include <memory>
#include <set>
#include <stdint.h>
#include <string>
#include <vector>

namespace SystemAbstractions {

    /**
     *  这个类定一个被当前进程启动的子进程，并且包括了两个进程通信的一些方法
     */
    class Subprocess {
    public:

        struct ProcessInfo {

            /**
             *  进程 id
             */
            unsigned int id;

            /**
             * 子进程运行的文件的路径
             */
            std::string image;

            /**
             *  TCP 端口
             */
            std::set< uint64_t > tcpServerPorts;

        };

    public:
        // 对象生命周期管理
        ~Subprocess() noexcept;
        Subprocess(const Subprocess&) = delete;
        Subprocess(Subprocess&&) noexcept;
        Subprocess& operator=(const Subprocess&) = delete;
        Subprocess& operator=(Subprocess&&) noexcept;

    public:
        Subprocess();

        unsigned int StartChild(
            std::string program,
            const std::vector< std::string >& args,
            std::function< void() > childExited,
            std::function< void() > childCrashed
        );

        static unsigned int StartDetached(
            std::string program,
            const std::vector< std::string >& args
        );

        bool ContactParent(std::vector< std::string >& args);

        static unsigned int GetCurrentProcessId();

        static std::vector< ProcessInfo > GetProcessList();

        static void Kill(unsigned int id);

    private:

        struct Impl;

        std::unique_ptr< Impl > impl_;
        
    };

}


#endif