#ifndef DEPFET_EXCEPTION_H
#define DEPFET_EXCEPTION_H

#include <stdexcept>

namespace DEPFET {
  class Exception: public std::runtime_error {
  public:
    Exception(const std::string& what): std::runtime_error(what) {}
  };
}

#endif
