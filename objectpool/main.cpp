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
		void Free(){}
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
	
	template <class UserClass, template<class> class Resource, template<class> class AcquireReleaseStrategie, class MutexStrategie>
	class Pool
	{
	public:
		Pool(size_t size, Creator<UserClass> creator): m_availableObjs(size)
		{
			for(auto &objPtr: m_availableObjs){
				objPtr = creator.Create();
			}
		}
		
		UserClass* Acquire()
		{
			MutexStrategie().lock();
			UserClass* obj = m_availableObjs.back();
			m_availableObjs.pop_back();
			MutexStrategie().unlock();
			return obj;
		}
		
		void Release(UserClass* obj)
		{
			MutexStrategie().lock();
			m_availableObjs.push_back(obj);
			MutexStrategie().unlock();
		}
	
		Creator<UserClass>& GetCreator(){
			return m_creator;
		}
		
		const Creator<UserClass>& GetCreator() const {
			return m_creator;
		}
		
		void ChangeCreator(Creator<UserClass> creator) {
			m_creator = std::move(creator);
		}
	private:
		std::list<UserClass*> m_availableObjs;
		Creator<UserClass> m_creator;
	};
}

int main()
{
	std::cout << "Hello, World!" << std::endl;
	return 0;
}

//
//ObjectPool<UserClass, AccessPolicy = RandomAcces> pool(size, creator);
//ObjectPool<UserClass, AccessPolicy = Map<UserId>> pool(size, creator);
//
//UserClass &pool.GetById("asdasd"); // only if AccesPolicy == Map
//UserClass &obj = pool.GetFreeRef();
//UserClass *obj2 = pool.GetFreePtr();
//
//pool.GetListOfFree(); // returns cosnt list of free objs. !!! Alarm! Multithreading !!!
//pool.GetListOfCaptive(); // returns cosnt list of captive objs. !!! Alarm! Multithreading !!!
//pool.Release();
//
//obj в работе.
//
//
//using MyClass = std::vector<int>;
//class MyCreator: public SuperDuperArchitecture::Creator<MyClass>
//{
//public:
//	MyCreator(size_t vectorSize, int vals):
//			m_vectorSize(vectorSize),
//			m_vals(vals)
//	{}
//	MyClass* Create() const override{
//		return (new MyClass(m_vectorSize, m_vals));
//	}
//private:
//	size_t m_vectorSize;
//	int m_vals;
//};
//
//int main()
//{
//	MyCreator myCreator(15, -239);
//	SuperDuperArchitecture::PoolObjectBind<std::vector<int>> objectBind(1000239, myCreator);
//	auto obj = objectBind.GetDataByRef();
//	int stop = 0;
//	std::cout << "Hello, World!" << std::endl;
//	return 0;
//}
//
//
//
//
//namespace
//{
//
//	template<class UserClass, template<class> class MemoryPolicy = MemoryNew>
//	class PoolObjectBind_elegant_but_bad
//	{
//	public:
//		template<class... UserClassConstructorParams>
//		PoolObjectBind_elegant_but_bad(size_t id, UserClassConstructorParams &&... params):m_id(id),
//		                                                                                   m_free(true)
//		{
//			m_data = MemoryPolicy<UserClass>::New(params...);
//		};
//
//		const UserClass *GetDataPtr() const
//		{
//			return m_data;
//		};
//
//		UserClass *GetDataPtr()
//		{
//			return m_data;
//		};
//
//		const UserClass &GetDataRef() const
//		{
//			return *m_data;
//		};
//
//		UserClass &GetDataRef()
//		{
//			return *m_data;
//		};
//
//		size_t GetId() const
//		{
//			return m_id;
//		};
//
//		bool IsFree() const
//		{
//			return m_free;
//		};
//
//		void SetFree()
//		{
//			m_free = true;
//		}
//
//		void SetCaptive()
//		{
//			m_free = false;
//		}
//
//	private:
//		size_t m_id;
//		bool m_free;
//		UserClass *m_data = nullptr;
//	};
//
//}
//
//
//namespace SuperArchitecture
//{
//
//	/////
//	/// То, что отображается на мониторе и идет в видеокарту
//	class GraphicObject
//	{};
//
//	//////
//	/// Все, что может взаимодействовать с логикой процесса.
//	class GameObject
//	{
//	public:
//		GameObject(uint32_t objectId) : m_id(objectId){
//		}
//
//		uint32_t GetId() const {
//			return m_id;
//		}
//		void SetId(uint32_t id) {
//			m_id = id;
//		}
//		virtual ~GameObject() = default;
//	private:
//		uint32_t m_id;
//	};
//
//	template <template <class> class Container, template <class> class Memory>
//	class GameObjectPool
//	{
//	public:
//		typedef Memory<GameObject> Memory_t;
//		typedef Container<Memory_t> Containger_t;
//		GameObjectPool(){}
//		void AddObject();
//	private:
//		Containger_t m_objectPool;
//	};
//
//	class Hero : public GameObject
//	{
//	public:
//		Hero(uint32_t objectId): GameObject(objectId){}
//		Hero(uint32_t objectId, const std::string &name): GameObject(objectId), m_name(name){}
//
//		std::string GetName() const {
//			return m_name;
//		}
//		void SetName(const std::string &name){
//			m_name = name;
//		};
//	private:
//		std::string m_name;
//		std::shared_ptr<GraphicObject> m_visualisation;
//
//	};
//}
