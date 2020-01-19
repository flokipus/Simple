//
// Created by mithrandir on 18.01.2020.
//

#ifndef SIMPLE_THREADMODEL_H
#define SIMPLE_THREADMODEL_H

#include <mutex>

namespace simple{
    class SinglethreadModel
    {
    public:
        struct Locker{};
    };

    class MultithreadModel
    {
    public:
        typedef Locker std::lock_guard;
    };
}

#endif //SIMPLE_THREADMODEL_H
