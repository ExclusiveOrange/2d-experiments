#include <type_traits>

template<class F, class ... Args>
constexpr auto takes = std::is_invocable_v<F, Args...>;

template<class Result, class F, class ... Args>
constexpr auto returns = std::is_same_v<Result, std::invoke_result_t<F(Args...)>>;

template<class F>
using result_t = typename std::invoke_result_t<F()>;

template<class F, class ... Args, class Result>
concept func = takes<F, Args...> && returns<Result, F, Args...>;

template<class F, class R>
concept action = std::is_same_v<R, result_t<std::remove_reference_t<F>>>;

