//
// Created by mithrandir on 19.01.2020.
//

#ifndef SIMPLE_THREADMODEL_H
#define SIMPLE_THREADMODEL_H

#include <mutex>

namespace simple
{
    //////
    /// Empty class. It does nothing
    class SinglethreadModel
    {
    public:
        void lock(){}
        void unlock(){}
    };

    //////
    /// It is used for multithread case.
    using MultithreadModel = std::mutex;
}


#endif //SIMPLE_THREADMODEL_H
