#include <type_traits>

//------------------------------------------------------------------------------
// template parameter deduction

// for struct return_type, thanks: https://stackoverflow.com/a/16680041
template<class F>
struct return_type;

template<class R, class...Args>
struct return_type<R(Args...)> {using type = R;};

//------------------------------------------------------------------------------
// aliases

template<class F>
using return_t = typename return_type<std::remove_reference_t<F>>::type;

//------------------------------------------------------------------------------
// values

template<class F>
constexpr bool parameterless_v = std::is_invocable_v<F()>;

template<class F, class ... Args>
constexpr auto takes = std::is_invocable_v<F, Args...>;

//------------------------------------------------------------------------------
// concepts

// invocable F that takes zero arguments and returns R
template<class F, class R>
concept Action = parameterless_v<F> && std::is_same_v<return_t<F>, R>;

// invocable F that takes Args... and returns R
template<class F, class ... Args, class R>
concept Function = std::is_invocable_v<F, Args...> && std::is_same_v<return_t<F>, R>;

// invocable F that takes zero or more arguments and returns R
template<class F, class R>
concept Returns = std::is_same_v<return_t<F>, R>;
