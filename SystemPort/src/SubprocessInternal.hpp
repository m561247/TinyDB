#ifndef SYSTEM_PORT_SUBPROCES_INTERNAL_HPP
#define SYSTEM_PORT_SUBPROCES_INTERNAL_HPP

/**
 * @file SubprocessInternal.hpp
 *
 * This module declares functions used internally by the Subprocess module
 * of the SystemAbstractions library.
 *
 * © 2020 by LiuJ
 */

namespace SystemAbstractions {

    /**
     *  关掉当前进程打开所有的文件句柄，给定的句柄除外
     */
    void CloseAllFilesExcept(int keepOpen);

}

#endif /* SYSTEM_PORT_SUBPROCES_INTERNAL_HPP */