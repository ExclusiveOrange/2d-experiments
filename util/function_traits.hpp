#include <tuple>
#include <type_traits>

// thanks: https://stackoverflow.com/a/2565037
// That post provided the basic idea of how to catch a lambda with template specialization,
// however it didn't quite work: some commentors pointed out it is necessary to use const
// on the member function pointer specialization.
// Atlee added:
//   normal function specialization
//   function_traits::args_t,
//   same_rets_v, same_args_v, same_fn_sigs_v,
//   and concept Function.

template<class T>
struct function_traits // matches when T=X or T=lambda
{
  using ret_t = typename function_traits<decltype(&T::operator())>::ret_t;
  using args_t = typename function_traits<decltype(&T::operator())>::args_t;
};

template<class R, class C, class... A>
struct function_traits<R (C::*)(A...) const> // matches X::operator() but not lambda::operator()
{
  using ret_t = R;
  using args_t = std::tuple<A...>;
};

template<class R, class... A>
struct function_traits<R (*)(A...)> // matches function pointer
{
  using ret_t = R;
  using args_t = std::tuple<A...>;
};

// Atlee: added everything below

template<class R, class... A>
struct function_traits<R(A...)> // matches normal function
{
  using ret_t = R;
  using args_t = std::tuple<A...>;
};

template<class FnA, class FnB>
constexpr bool same_rets_v = std::is_same_v<typename function_traits<FnA>::ret_t, typename function_traits<FnB>::ret_t>;

template<class FnA, class FnB>
constexpr bool same_args_v = std::is_same_v<typename function_traits<FnA>::args_t, typename function_traits<FnB>::args_t>;

template<class FnA, class FnB>
constexpr bool same_fn_sigs_v = same_rets_v<FnA, FnB> && same_args_v<FnA, FnB>;

template<class FnA, class FnB>
concept Function = same_fn_sigs_v<std::remove_reference_t<FnA>, std::remove_reference_t<FnB>>;