#include <iostream>
#include <vector>
#include <list>
#include <string>
#include <memory>

namespace SuperDuperArchitecture
{
	template <class T>
	class MemoryNew
	{
	public:
		template<class... ConstructorParams>
		static T* New(ConstructorParams&&... params){
			T* ptr = new T(params...);
			return ptr;
		};
	};
	

	
	template <class UserClass>
	class Creator
	{
	public:
		virtual UserClass* Create() const{
			return (new UserClass);
		};
		
		virtual ~Creator() = default;
	};
	
	template <class UserClass>
	class PoolObjectBind{
	public:
		PoolObjectBind(size_t id,
		               const Creator<UserClass> &creator = Creator<UserClass>()):
				m_id(id),
				m_free(true)
		{
			m_data = creator.Create();
		};
		
		const UserClass* GetDataByPtr() const{
			return m_data;
		};
		UserClass* GetDataByPtr(){
			return m_data;
		};
		const UserClass& GetDataByRef() const{
			return *m_data;
		};
		UserClass& GetDataByRef(){
			return *m_data;
		};
		size_t GetId() const{
			return m_id;
		};
		bool IsFree() const{
			return m_free;
		};
		void SetFree(){
			m_free = true;
		}
		void SetCaptive(){
			m_free = false;
		}
	private:
		size_t m_id;
		bool m_free;
		UserClass* m_data = nullptr;
	};
	
	
//
//	template <class UserClass>
//	class ObjectGod
//	{
//	public:
//		typedef UserClass UserClass_t;
//		template <class... Parameters>
//		static UserClass* Create(Parameters&&... parameters);
//
//		template <class... Parameters>
//		static UserClass* Destroy(Parameters&&... parameters);
//	};
	
	template <class UserClass, template<class> class MemoryPolicy, template<class> class GetPolicy>
	class ObjectPool
	{
		ObjectPool(size_t size, Creator<UserClass> creator)
		{
			for(size_t i = 0; i < size; i++){
				m_availableObjs.push_back(PoolObjectBind<UserClass>(i, creator));
			}
		}
		
		PoolObjectBind<UserClass>* GetFreeObj()
		{
			auto &obj = m_availableObjs.back();
			m_availableObjs.pop_back();
			return &obj;
		}
		
		void ReleaseObject(PoolObjectBind<UserClass>* obj)
		{
			m_availableObjs.emplace_back(*obj);
		}
		
	private:
		std::list<PoolObjectBind<UserClass>> m_availableObjs;
		std::vector<PoolObjectBind<UserClass>> m_availableObjsVector;
		//std::list<PoolObjectBind<UserClass>> m_inUseOjbs;
	};
	
	
	template <class UserClass, template<class> class Resource, template<class> class AcquireReleaseStrategie>
	class EzPool
	{
	public:
		EzPool(size_t size, Creator<UserClass> creator): m_availableObjs(size)
		{
			for(auto &objPtr: m_availableObjs){
				objPtr = creator.Create();
			}
		}
		
		UserClass* Acquire()
		{
			UserClass* obj = m_availableObjs.back();
			m_availableObjs.pop_back();
			return obj;
		}
		
		void Release(UserClass* obj)
		{
			m_availableObjs.push_back(obj);
		}
		
	private:
		std::list<UserClass*> m_availableObjs;
	};
}

#include <mutex>

namespace
{
	template<class UserType>
	class Resource
	{
	public:
		UserType* GetDataPtr(){
			return m_data;
		}
		
		Resource():
				m_data(nullptr)
		{ }
		
		Resource(const Resource&other):m_data(other.m_data)
		{ }
	private:
		UserType* m_data;
	};
	
	template<class UserType>
	class ResourceStrategy
	{
	public:
		ResourceStrategy(size_t size, SuperDuperArchitecture::Creator<UserType> creator)
		{ }
		Resource<UserType> Acquire()
		{
			auto resource = m_available.back();
			m_available.pop_back();
			return resource;
		};
		void Release(Resource<UserType> obj)
		{
			m_available.push_back(obj);
		}
	private:
		std::list<Resource<UserType>> m_available;
	};
	
	
	class NoMutexStrategy
	{
	public:
		void Lock(){}
		void Unlock(){}
	private:
	};
	
	class SimpleMutexStrategy
	{
	public:
		void Lock(){
			m_mutex.lock();
		}
		void Free(){
			m_mutex.unlock();
		}
	private:
		std::mutex m_mutex;
	};
	
	template<class UserType, template<class> class ResourceStrategy = ResourceStrategy, class MutexStrategy = NoMutexStrategy>
	class Pool
	{
	public:
		Pool(size_t size);
		Resource<UserType> Acquire()
		{
			m_mutexStrategy.lock();
			auto resource = m_resourceStrategy.Acquire();
			m_mutexStrategy.unlock();
			return resource;
		}
		void Release(Resource<UserType> resource)
		{
			m_mutexStrategy.lock();
			m_resourceStrategy.Release(resource);
			m_mutexStrategy.unlock();
		}
		
		Resource<UserType> AcquireById(size_t id)
		{
			
		}
	private:
		MutexStrategy m_mutexStrategy;
		ResourceStrategy<UserType> m_resourceStrategy;
	};
}


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