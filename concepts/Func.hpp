#include <type_traits>

template< class F, class ... Args >
constexpr auto Takes = std::is_invocable_v< F, Args... >;

template< class Result, class F, class ... Args >
constexpr auto Returns = std::is_same_v< Result, std::invoke_result_t< F( Args... )>>;

template< class F, class ... Args, class Result >
concept Func = Takes< F, Args... > && Returns< Result, F, Args... >;

