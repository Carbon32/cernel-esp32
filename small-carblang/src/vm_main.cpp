#include "core/runtime.hpp"
#include "core/bytecode.hpp"
#include <fstream>

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: small_carblang <bytecode.crbc>" << std::endl;
        return 64;
    }

    std::ifstream input(argv[1], std::ios::binary);
    if (!input)
    {
        std::cerr << "Failed to open bytecode file: " << argv[1] << std::endl;
        return 74;
    }

    try
    {
        Chunk chunk = read_bytecode(input);
        VM vm;
        vm.init_globals();
        vm.interpret(chunk);
        return 0;
    }
    catch (const std::exception &exception)
    {
        std::cerr << exception.what() << std::endl;
        return 70;
    }
}
