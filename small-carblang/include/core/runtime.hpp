#ifndef RUNTIME_HPP
#define RUNTIME_HPP

#include "deps.hpp"
#include "opcode.hpp"
#include "token_type.hpp"
#include "forward.hpp"
#include "ang.hpp"
#include "native.hpp"
#include "define.hpp"
#include "module.hpp"
#include "json.hpp"
#include "csv.hpp"
#include "profile.hpp"
#include "token.hpp"
#include "error.hpp"

std::shared_ptr<BoundMethod> make_native_method(Value value, NativeMethod method);

#include "vm.hpp"

#endif
