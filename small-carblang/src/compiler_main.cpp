#include "core/core.hpp"
#include "core/bytecode.hpp"
#include <filesystem>
#include <fstream>
#include <string_view>

int main(int argc, char **argv)
{
    if (argc == 2 && std::string_view(argv[1]) == "-i")
    {
        Carblang carblang(true);
        carblang.interactive();
        return 0;
    }

    if (argc != 2 && !(argc == 4 && std::string_view(argv[2]) == "-o"))
    {
        std::cerr << "Usage: small_carblangc [-i | <source.crb> [-o <output.crbc>]]" << std::endl;
        return 64;
    }

    const std::filesystem::path source_path(argv[1]);
    std::filesystem::path output_path = argc == 4
        ? std::filesystem::path(argv[3])
        : source_path;
    if (argc != 4)
        output_path.replace_extension(".crbc");

    std::ifstream source(source_path, std::ios::binary);
    if (!source)
    {
        std::cerr << "Failed to open source file: " << source_path.string() << std::endl;
        return 74;
    }

    std::string code((std::istreambuf_iterator<char>(source)),
                     std::istreambuf_iterator<char>());
    Carblang carblang;
    try
    {
        auto chunk = carblang.compile_source(std::move(code));
        if (!chunk)
            return 65;
        std::ofstream output(output_path, std::ios::binary | std::ios::trunc);
        if (!output)
        {
            std::cerr << "Failed to create bytecode file: " << output_path.string() << std::endl;
            return 74;
        }
        write_bytecode(output, *chunk);
        std::cout << "Wrote bytecode: " << output_path.string() << std::endl;
        return 0;
    }
    catch (const std::exception &exception)
    {
        std::cerr << exception.what() << std::endl;
        return 65;
    }
}