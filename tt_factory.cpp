#include <memory>
#include <type_traits>
 


struct TypeA{};
struct TypeB{};

struct FooTrait {};
struct BooTrait {};

template <typename T, typename U>
typename std::enable_if<std::is_same<U, FooTrait>::value, std::shared_ptr<T> >::type
make()  
{
    
    return std::make_shared<T>();
}

template <typename T, typename U>
typename std::enable_if<std::is_same<U, BooTrait>::value, std::unique_ptr<T>>::type
make()
{
    return std::unique_ptr<T>(new T);
}


int main()
{
    auto m = make<TypeA, FooTrait>();    
    auto n = make<TypeA, BooTrait>();

    return 0;
}


