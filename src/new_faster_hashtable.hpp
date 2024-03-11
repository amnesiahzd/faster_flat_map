#include <utility>

template<typename Functor>
struct function_wrapper : public Functor {
    using Functor::Functor;
    function_wrapper(const Functor& other_functor) : Functor(other_functor) {}

    template<typename... Args>
    decltype(auto) operator()(Args&&... args) {
        return Functor::operator()(std::forward<Args>(args)...);
    }

};

template<typename Result, typename... Args>
struct function_wrapper<Result (*)(Args...)> {
    using function_ptr = Result (*)(Args...);
    function_ptr _function;

    explicit function_wrapper(function_ptr& other_ptr) : _function(other_ptr) {}

    Result operator()(Args&&... args) {
        return _function(std::forward<Args>(args)...);
    }

    operator function_ptr &() {
        return _function;
    }
    operator const function_ptr &() {
        return _function;
    }
};
