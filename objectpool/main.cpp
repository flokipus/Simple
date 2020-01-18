#include <iostream>
#include <vector>
#include <list>
#include <string>
#include <memory>

#include <memory>

////
// The simpler - the better
namespace simple
{
	template <class UserClass>
	class Creator
	{
	public:
		virtual UserClass* Create() const{
			return (new UserClass);
		};
		
		virtual ~Creator() = default;
	};

    /////
    /// Сырое выделение ресурсов: если недостаточно памяти, то вызывается exception
    /// Нет проверки на то, что возвращаемый ресурс действительно из данного пула
    /// \tparam UserClass
    /// \tparam MemoryContainer     требования: наличие конструктора с сигнатурой (size_t, UserType*);
    ///                             наличие метода: void resize()
    ///                             наличие метода: size_t size()
    ///                             наличие метода: Resource& operator[](size_t)
    template<class UserClass>
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

        AcquireReleaseRaw(size_t reservedSize,
                          std::shared_ptr<Creator<UserClass>> creator): m_pool(reservedSize, Resource()),
                                                                        m_creator(creator)
        {
            for(auto &resource: m_pool){
                resource.m_ptrToData = creator->Create();
            }
        }

        Resource Acquire()
        {
            if(m_pool.size() > 0){
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
        }
    private:
        std::vector<Resource> m_pool;
        std::shared_ptr<Creator<UserClass>> m_creator;
    };

	template <class UserClass, template<class> class AcquireReleaseStrategy = AcquireReleaseRaw, class MutexStrategy = NoMutexStrategy>
	class ObjectPoolHolder
	{
	public:
        typedef typename AcquireReleaseStrategy<UserClass>::resource_t resource_t;

		ObjectPoolHolder(size_t size, Creator<UserClass> creator = Creator<UserClass>()):
		    m_availableObjs(size, std::make_shared<Creator<UserClass>>(creator))
		{
		}
		
		auto Acquire()
		{
			MutexStrategy().Lock();
			auto obj = m_availableObjs.Acquire();
			MutexStrategy().Unlock();
			return obj;
		}
		
		void Release(resource_t obj)
		{
			MutexStrategy().Lock();
			m_availableObjs.Release(obj);
			MutexStrategy().Unlock();
		}

		auto AcquireByUserId();

	private:
        AcquireReleaseStrategy<UserClass> m_availableObjs;
	};


}

int main()
{
    using mytype = std::vector<int>;
    simple::ObjectPoolHolder<mytype> pool(10);
    auto res1 = pool.Acquire();
    auto res2 = pool.Acquire();
    auto &data1 = res1.GetObjectByRef();
    data1.resize(100);
    pool.Release(res2);
	std::cout << "Hello, World!" << std::endl;
	return 0;
}