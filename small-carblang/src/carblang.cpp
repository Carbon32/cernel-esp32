#include "core/core.hpp"
#include <fstream>

int main(int argc, char **argv)
{
    if (argc == 2 && std::string_view(argv[1]) == "-i")
    {
        Carblang carblang(true);
        carblang.interactive();
        return 0;
    }

    if (argc != 2)
    {
        std::cerr << "Usage: small_carblang [-i | <script>]" << std::endl;
        return 1;
    }

    std::ifstream script(argv[1], std::ios::binary);
    if (!script)
    {
        std::cerr << "Failed to open script: " << argv[1] << std::endl;
        return 1;
    }

    std::string source((std::istreambuf_iterator<char>(script)),
                       std::istreambuf_iterator<char>());
    Carblang carblang;
    carblang.start(std::move(source));
    return 0;
}
