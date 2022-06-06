#include <exception>
#include <string>
#include <type_traits>

// struct ErrorOrSuccess
// struct Error
// struct Success
//
// used to implement fluent error handling for simple error/success cases where the error is a simple string
//
//======================================================================================================================
// example:
//----------------------------------------------------------------------------------------------------------------------
//   ErrorOrSuccess doThing( int x )
//   {
//     if( x > 5 )
//       return Success();
//     else
//       return Error( "x <= 5 is an error" );
//   }
//
//   void tryToDoThing( int x )
//   {
//     doThing( x )
//         .error( []( auto errormsg ) { std::cerr << "error: " << errormsg << std::endl; } )
//         .success( [] { std::cout << "success!" << std::endl; } );
//   }
//======================================================================================================================

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
  MaybeSuccess error( F &&f ) requires std::is_invocable_v<F, std::string>
  {
    if( invoked )
      throw std::exception( "ErrorOrSuccess.error was called twice" );

    bool success = errormsg.empty();

    if( !success )
      f( errormsg );

    errormsg.clear(); // don't need anymore
    invoked = true;

    return MaybeSuccess{ success };
  }

private:
  std::string errormsg;
  bool invoked{};
};

struct [[nodiscard]] Error : ErrorOrSuccess
{
  explicit Error( std::string errormsg ) : ErrorOrSuccess{ std::move( errormsg ) } {}
};

struct [[nodiscard]] Success : ErrorOrSuccess
{
  explicit Success() : ErrorOrSuccess{} {}
};

