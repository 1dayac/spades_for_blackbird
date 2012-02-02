
#pragma once
#include "stacktrace.hpp"
#include "bits/signum.h"


struct segfault_handler
    : boost::noncopyable
{
    typedef boost::function<void ()> callback_t;
    typedef void (*seg_handler_t)(int);

    segfault_handler(callback_t const& cb = 0)
    {
        if (callback() != 0)
            throw std::runtime_error("failed to initialize segfault_handler, it has been already initialized");

        callback() = cb;
        old_func_  = signal(SIGSEGV, &segfault_handler::handler);
    }

    ~segfault_handler()
    {
        callback() = 0;
        signal(SIGSEGV, old_func_);
    }

private:
    static callback_t& callback()
    {
        static callback_t cb = 0;
        return cb;
    }

    static void handler(int signum)
    {
        if (signum == SIGSEGV)
        {
            std::cerr << "The program was terminated by segmentation fault" << std::endl;
            print_stacktrace();

            callback()();
        }

        signal(signum, SIG_DFL);
        kill  (getpid(), signum);
    }


private:
    seg_handler_t  old_func_;
};
