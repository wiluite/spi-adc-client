#pragma once
#include <sstream>
#include <iostream>
#include <iomanip>

//Stroustrup 4, 28.6.1

static inline void print_to_obuf(std::ostream &) noexcept
{}

template <typename T>
static inline void log_impl(std::ostream & log, T && val)
{
    log << val;
}

template <class First, class ... Rest>
static inline void print_to_obuf (std::ostream & log, First && first, Rest && ... rest)
{
    log_impl(log, std::forward<First>(first));
    print_to_obuf(log, std::forward<Rest>(rest) ...);
}

class Log_Wrapper
{
public:
    template <typename ... T>
    explicit Log_Wrapper(T && ... args)
    {
        std::stringstream log {};
        print_to_obuf (log, std::forward<T>(args) ...);
        std::cout << log.str() << std::endl;
    }
};
