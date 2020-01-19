//
// Created by mithrandir on 19.01.2020.
//

#ifndef SIMPLE_OBJECTPOOL_H
#define SIMPLE_OBJECTPOOL_H

#include <threadmodel/threadmodel.h>

namespace simple
{
    //////
    /// Has two methods:
    ///     AcquireReleaseStrategy::resource_t Acquire();
    ///     void Release(AcquireReleaseStrategy::resource_t).
    /// Their behavior is specialazied by class AcquireReleaseStrategy.
    /// resource_t object must contain method GetObjectByRef() that return UserClass&.
    /// \tparam UserClass                   Any.
    /// \tparam AcquireReleaseStrategy      1. It must have constructor from size_t:
    ///                                         AcquireReleaseStrategy(size_t);
    ///                                     2. Must contain two methods:
    ///                                         resource_t Acquire();
    ///                                         void Release(resource_t);
    ///                                     3. And there must be defined a type:
    ///                                         AcquireReleaseStrategy::resource_t,
    ///                                     that has method
    ///                                         UserClass& GetObjectByRef();
    /// \tparam ThreadStrategy
    template <class UserClass,
            class AcquireReleaseStrategy,
            class ThreadStrategy>
    class ObjectPoolHolder
    {
    public:
        typedef typename AcquireReleaseStrategy::resource_t resource_t;

        ObjectPoolHolder(size_t size):
                m_availableObjs(size)
        {
        }

        resource_t Acquire()
        {
            m_threadStrategy.lock();
            auto obj = m_availableObjs.Acquire();
            m_threadStrategy.unlock();
            return obj;
        }

        void Release(resource_t obj)
        {
            m_threadStrategy.lock();
            m_availableObjs.Release(obj);
            m_threadStrategy.unlock();
        }

    protected:
        AcquireReleaseStrategy m_availableObjs;
        ThreadStrategy m_threadStrategy;
    };

    /////
    /// Сырое выделение ресурсов: выделяется определенное количество указателей на объекты в памяти (по умолчанию, все
    /// они называются свободными); в ходе работы объекты могут становиться занятыми (метод Acquire()):
    /// если пользователь запросил больше объектов, чем осталось свободных, то вызывается exception
    /// NB!!! Нет проверки на то, что возвращаемый ресурс действительно из данного пула. Если требуется не допустить
    /// такого, то паттерны monostate и numerate types в помощь.
    /// \tparam UserClass
    /// \tparam CreateStrategy
    /// \tparam OnReleaseStrategy
    template<class UserClass,
            class CreateStrategy,
            class OnReleaseStrategy>
    class AcquireReleaseRaw;


    template <class UserClass>
    class NewCreator
    {
    public:
        static UserClass* Create(){
            return (new UserClass);
        };
    };

    template <class UserClass>
    class DoNothing
    {
    public:
        static void OnRelease(UserClass*){}
    };

    template<class UserClass,
            class CreateStrategy = NewCreator<UserClass>,
            class OnReleaseStrategy = DoNothing<UserClass>>
    class AcquireReleaseRaw
    {
    public:
        class Resource;
        typedef Resource resource_t;

        class Resource
        {
        public:
            UserClass& GetObjectByRef(){
                return *m_ptrToData;
            }
        private:
            UserClass* m_ptrToData = nullptr;
            friend class AcquireReleaseRaw;
        };

        AcquireReleaseRaw(size_t reservedSize):
                m_pool(reservedSize, Resource())
        {
            for(auto &resource: m_pool){
                resource.m_ptrToData = CreateStrategy::Create();
            }
        }

        Resource Acquire()
        {
            if(not m_pool.empty()){
                auto resource = m_pool.back();
                size_t newSize = m_pool.size() - 1;
                m_pool.resize(newSize);
                return resource;
            }
            else{
                throw "No memory available";
            }
        }

        void Release(Resource resource)
        {
            size_t newSize = m_pool.size() + 1;
            m_pool.resize(newSize);
            m_pool[newSize-1] = resource;
            OnReleaseStrategy::OnRelease(resource.m_ptrToData);
        }
    private:
        std::vector<Resource> m_pool;
    };

    /// Some predefined templates
    template<class UserClass, class AcquireReleaseStrategy>
    using SingleThreadedObjectPool = ObjectPoolHolder<UserClass, AcquireReleaseStrategy, SinglethreadModel>;
    template<class UserClass>
    using StandartObjectPool = ObjectPoolHolder<UserClass, AcquireReleaseRaw<UserClass>, SinglethreadModel>;
}


#endif //SIMPLE_OBJECTPOOL_H
