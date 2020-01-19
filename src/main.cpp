#include <iostream>
#include <vector>
#include <list>

#include <objectpool/objectpool.h>

////
// The simpler - the better


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

using MyPool = simple::SingleThreadedObjectPool<mytype, MyAcRelStrategy>;

struct Fallback{ int fff = 239;};
int Fallback::* a = nullptr;

#include <iostream>

int main()
{
    int Fallback::* c = &Fallback::fff;
    Fallback f;
    std::cout << f.fff << std::endl;
    f.*c = 14;
    std::cout << f.fff << std::endl;
    std::cout << typeid(c).name() << std::endl;
    //a = &bb;
    MyPool pool(100);
    auto res1 = pool.Acquire();
    auto res2 = pool.Acquire();
    auto &data1 = res1.GetObjectByRef();
    data1.resize(100);
    pool.Release(res2);
	std::cout << "Hello, World!" << std::endl;
	return 0;
}