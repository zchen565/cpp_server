//例子为官方代码
//https://en.cppreference.com/w/cpp/coroutine/coroutine_handle
#include <coroutine>
#include <iostream>
#include <optional>
 
template<std::movable T> //C++20 Concept语法，T必须是movable的。此处不关键，可以当作<typename T>
class Generator
{
public:
    struct promise_type // promise_type必须包含以下接口，其中yield_value为co_yield
    {
        Generator<T> get_return_object() //返回一个可以和协程交互的对象,详情问gpt4
        {
            return Generator{Handle::from_promise(*this)}; 
        }
        static std::suspend_always initial_suspend() noexcept //创建后直接挂起
        {
            return {};
        }
        static std::suspend_always final_suspend() noexcept // 暂时不太明白
        {
            return {};
        }
        std::suspend_always yield_value(T value) noexcept //co_yield 特有
        {
            current_value = std::move(value);
            return {};
        }
        // Disallow co_await in generator coroutines.
        void await_transform() = delete;

        // 修饰 unhandled_exception，遇到异常直接结束协程，不会返回
        [[noreturn]]
        static void unhandled_exception() { throw; }
 
        std::optional<T> current_value; // C++17 存放协程数据信息
    };
 
    using Handle = std::coroutine_handle<promise_type>; //这个是通俗用法
 

    // 类的六大金刚
    explicit Generator(const Handle coroutine) :
        m_coroutine{coroutine}
    {}
 
    Generator() = default;
    ~Generator()
    {
        if (m_coroutine)
            m_coroutine.destroy();
    }
 
    Generator(const Generator&) = delete;
    Generator& operator=(const Generator&) = delete;
 
    Generator(Generator&& other) noexcept :
        m_coroutine{other.m_coroutine}
    {
        other.m_coroutine = {};
    }
    Generator& operator=(Generator&& other) noexcept
    {
        if (this != &other)
        {
            if (m_coroutine)
                m_coroutine.destroy();
            m_coroutine = other.m_coroutine;
            other.m_coroutine = {};
        }
        return *this;
    }
 


    // Range-based for loop support.
    class Iter
    {
    public:
        void operator++()
        {
            m_coroutine.resume();
        }
        const T& operator*() const
        {
            return *m_coroutine.promise().current_value;
        }
        bool operator==(std::default_sentinel_t) const
        {
            return !m_coroutine || m_coroutine.done();
        }
 
        explicit Iter(const Handle coroutine) :
            m_coroutine{coroutine}
        {}
 
    private:
        Handle m_coroutine;
    };
 
    Iter begin()
    {
        if (m_coroutine)
            m_coroutine.resume();
        return Iter{m_coroutine};
    }
 
    std::default_sentinel_t end() { return {}; }
 
private:
    Handle m_coroutine; //协程句柄 coroutine_handle
};
 
template<std::integral T> //同上，T为整数类型或枚举类型
Generator<T> range(T first, const T last) //
{
    while (first < last)
        co_yield first++;
}
 
int main()
{
    for (const char i : range(65, 91))
        std::cout << i << ' ';
    std::cout << '\n';
}