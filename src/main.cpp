#include <iostream>
#include <utility>

#include <SDL.h>

struct Tester
{
  Tester() = default;
};

Tester doit()
{
  return Tester{};
}

namespace
{
  struct [[nodiscard]] ErrorOrSuccess
  {
    struct [[nodiscard]] MaybeSuccess
    {
      explicit MaybeSuccess( bool success ) : _success{ success } {}

      template< class F >
      void success( F &&f ) requires std::is_invocable_v<F>
      {
        if( _success )
          f();
      }
    private:
      bool _success;
    };

    ErrorOrSuccess() = default;
    explicit ErrorOrSuccess( std::string errormsg ) : errormsg{ std::move( errormsg ) } {}

    template< class F >
    [[nodiscard]] MaybeSuccess error( F &&f ) requires std::is_invocable_v<F, std::string>
    {
      if( invoked )
        throw std::exception("MaybeError.error was called twice and that is bad don't do it");

      bool success = errormsg.empty();

      if( !success )
        f( errormsg );

      errormsg.clear(); // don't need anymore
      invoked = true;

      return MaybeSuccess{success};
    }

  private:
    std::string errormsg;
    bool invoked{};
  };

  ErrorOrSuccess error( std::string errormsg ) { return ErrorOrSuccess{ std::move( errormsg ) }; }
  const ErrorOrSuccess success{};

  ErrorOrSuccess init(int x)
  {
    return x < 0 ? error("negative") : success;
  }
}

int main( int argc, char *argv[] )
{
  init(1)
      .error( []( auto errormsg ) { std::cerr << "error: " << errormsg << std::endl; } )
      .success( [] { std::cout << "success" << std::endl; } );

  return 0;
}
