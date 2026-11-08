#ifndef BYTECODE_HPP
#define BYTECODE_HPP

#include "runtime.hpp"

// I can squeeze a little bit of performance out of the language if I rely more on bytecode files
void write_bytecode(std::ostream &output, const Chunk &chunk);
Chunk read_bytecode(std::istream &input);

#endif
