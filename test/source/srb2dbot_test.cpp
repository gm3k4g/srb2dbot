#include "lib.hpp"

auto main() -> int
{
  auto const lib = library {};

  return lib.name == "srb2dbot" ? 0 : 1;
}
