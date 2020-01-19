#include <iostream>
#include <vector>
#include <list>
#include <string>
#include <memory>

#include <memory>
#include <mutex>
#include <functional>


////
// The simpler - the better
namespace simple
{

    class SinglethreadModel
    {
    public:
        void lock(){}
        void unlock(){}
    };

    using MultithreadModel = std::mutex;


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

    /////
    /// Сырое выделение ресурсов: выделяется определенное количество указателей на объекты в памяти (по умолчанию, все
    /// они называются свободными); в ходе работы объекты могут становиться занятыми (метод Acquire()):
    /// если пользователь запросил больше объектов, чем осталось свободных, то вызывается exception
    /// NB!!! Нет проверки на то, что возвращаемый ресурс действительно из данного пула. Если требуется не допустить
    /// такого, то паттерны monostate и numerate types в помощь.
    /// \tparam UserClass
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

	template <class UserClass,
	          class AcquireReleaseStrategy = AcquireReleaseRaw<UserClass>,
	          class ThreadStrategy = SinglethreadModel>
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

		/// wip
        //resource_t AcquireByUserId(typename AcquireReleaseStrategy::Id_t){}

	private:
        AcquireReleaseStrategy m_availableObjs;
        ThreadStrategy m_threadStrategy;
	};


}

using mytype = std::vector<int>;

class MyCreator
{
public:
    static mytype* Create(){
        //std::cout << "vector<int>(0,100) is created" << std::endl;
        return new mytype (0, 100);
    }
};

class  MyOnRelease
{
public:
    static void OnRelease(mytype*){
        std::cout << "vector<int>(0,100) is released" << std::endl;
    }
};

using MyAcRelStrategy = simple::AcquireReleaseRaw<mytype, MyCreator, MyOnRelease>;

using MyPool = simple::ObjectPoolHolder<mytype, MyAcRelStrategy>;

int main()
{
    MyPool pool(100);
    auto res1 = pool.Acquire();
    auto res2 = pool.Acquire();
    auto &data1 = res1.GetObjectByRef();
    data1.resize(100);
    pool.Release(res2);
	std::cout << "Hello, World!" << std::endl;
	return 0;
}