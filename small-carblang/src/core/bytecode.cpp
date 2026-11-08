#include "core/bytecode.hpp"
#include <cstring>

namespace
{
constexpr char magic[] = {'C', 'R', 'B', 'C'};
constexpr uint16_t format_version = 1;
constexpr uint32_t max_code_size = 64 * 1024 * 1024;
constexpr uint32_t max_string_size = 16 * 1024 * 1024;
constexpr uint32_t max_constants = 65536;
constexpr unsigned max_function_depth = 128;

enum class ConstantTag : uint8_t
{
    Null,
    Number,
    Boolean,
    String,
    Function,
    Ignore
};

void write_raw(std::ostream &out, const char *data, size_t size)
{
    out.write(data, static_cast<std::streamsize>(size));
    if (!out)
        throw std::runtime_error("Failed writing bytecode");
}

void read_raw(std::istream &in, char *data, size_t size)
{
    in.read(data, static_cast<std::streamsize>(size));
    if (in.gcount() != static_cast<std::streamsize>(size))
        throw std::runtime_error("Truncated bytecode file");
}

template<class T>
void write_le(std::ostream &out, T value)
{
    static_assert(std::is_unsigned_v<T>);
    for (size_t i = 0; i < sizeof(T); ++i)
        out.put(static_cast<char>((value >> (i * 8)) & 0xff));
    if (!out)
        throw std::runtime_error("Failed writing bytecode");
}

template<class T>
T read_le(std::istream &in)
{
    static_assert(std::is_unsigned_v<T>);
    T value = 0;
    for (size_t i = 0; i < sizeof(T); ++i)
    {
        const int byte = in.get();
        if (byte == std::char_traits<char>::eof())
            throw std::runtime_error("Truncated bytecode file");
        value |= static_cast<T>(static_cast<uint8_t>(byte)) << (i * 8);
    }
    return value;
}

void write_string(std::ostream &out, const std::string &value)
{
    if (value.size() > max_string_size)
        throw std::runtime_error("Bytecode string exceeds the format limit");
    write_le<uint32_t>(out, static_cast<uint32_t>(value.size()));
    write_raw(out, value.data(), value.size());
}

std::string read_string(std::istream &in)
{
    const uint32_t size = read_le<uint32_t>(in);
    if (size > max_string_size)
        throw std::runtime_error("Bytecode string exceeds the format limit");
    std::string value(size, '\0');
    read_raw(in, value.data(), size);
    return value;
}

void write_optional_string(std::ostream &out, const std::optional<std::string> &value)
{
    write_le<uint8_t>(out, value.has_value() ? 1 : 0);
    if (value)
        write_string(out, *value);
}

std::optional<std::string> read_optional_string(std::istream &in)
{
    const auto present = read_le<uint8_t>(in);
    if (present > 1)
        throw std::runtime_error("Invalid optional string marker in bytecode");
    if (!present)
        return std::nullopt;
    return read_string(in);
}

void write_chunk(std::ostream &out, const Chunk &chunk, unsigned depth);
Chunk read_chunk(std::istream &in, unsigned depth);

void write_function(std::ostream &out, const Function &function, unsigned depth)
{
    if (function.arity > UINT8_MAX || function.param_types.size() > UINT8_MAX)
        throw std::runtime_error("Function metadata exceeds the bytecode format limits");
    write_string(out, function.name);
    write_le<uint16_t>(out, static_cast<uint16_t>(function.arity));
    write_le<uint16_t>(out, static_cast<uint16_t>(function.param_types.size()));
    for (const auto &type : function.param_types)
        write_optional_string(out, type);
    write_optional_string(out, function.return_type);
    write_string(out, function.owner_class);
    write_le<uint8_t>(out, function.is_public ? 1 : 0);
    if (function.declared_properties.size() > UINT16_MAX)
        throw std::runtime_error("Too many declared properties in bytecode function");
    write_le<uint16_t>(out, static_cast<uint16_t>(function.declared_properties.size()));
    for (const auto &[name, property] : function.declared_properties)
    {
        write_string(out, name);
        write_string(out, property.owner_class);
        write_le<uint8_t>(out, property.is_public ? 1 : 0);
        write_optional_string(out, property.type_name);
    }
    write_chunk(out, function.chunk, depth + 1);
}

std::shared_ptr<Function> read_function(std::istream &in, unsigned depth)
{
    std::string name = read_string(in);
    const auto arity = read_le<uint16_t>(in);
    const auto param_count = read_le<uint16_t>(in);
    if (arity > UINT8_MAX || param_count > UINT8_MAX || arity != param_count)
        throw std::runtime_error("Invalid function arity in bytecode");
    std::vector<std::optional<std::string>> param_types;
    param_types.reserve(param_count);
    for (uint16_t i = 0; i < param_count; ++i)
        param_types.push_back(read_optional_string(in));
    auto return_type = read_optional_string(in);
    std::string owner_class = read_string(in);
    const auto is_public = read_le<uint8_t>(in);
    if (is_public > 1)
        throw std::runtime_error("Invalid function visibility in bytecode");
    const auto property_count = read_le<uint16_t>(in);
    std::unordered_map<std::string, PropertyInfo> properties;
    properties.reserve(property_count);
    for (uint16_t i = 0; i < property_count; ++i)
    {
        std::string property_name = read_string(in);
        PropertyInfo property;
        property.owner_class = read_string(in);
        const auto property_public = read_le<uint8_t>(in);
        if (property_public > 1)
            throw std::runtime_error("Invalid property visibility in bytecode");
        property.is_public = property_public != 0;
        property.type_name = read_optional_string(in);
        properties.insert_or_assign(std::move(property_name), std::move(property));
    }
    Chunk chunk = read_chunk(in, depth + 1);
    auto function = std::make_shared<Function>(std::move(name), arity, std::move(chunk),
                                               std::move(param_types), std::move(return_type));
    function->owner_class = std::move(owner_class);
    function->is_public = is_public != 0;
    function->declared_properties = std::move(properties);
    return function;
}

void write_chunk(std::ostream &out, const Chunk &chunk, unsigned depth)
{
    if (depth > max_function_depth)
        throw std::runtime_error("Nested functions exceed the bytecode format limit");
    if (chunk.code.size() > max_code_size || chunk.constants.size() > max_constants)
        throw std::runtime_error("Chunk exceeds the bytecode format limits");
    write_le<uint32_t>(out, static_cast<uint32_t>(chunk.code.size()));
    if (!chunk.code.empty())
        write_raw(out, reinterpret_cast<const char *>(chunk.code.data()), chunk.code.size());
    write_le<uint32_t>(out, static_cast<uint32_t>(chunk.constants.size()));
    for (const Value &value : chunk.constants)
    {
        if (value_is<std::nullptr_t>(value))
            write_le<uint8_t>(out, static_cast<uint8_t>(ConstantTag::Null));
        else if (value_is<double>(value))
        {
            write_le<uint8_t>(out, static_cast<uint8_t>(ConstantTag::Number));
            uint64_t bits;
            const double number = value_get<double>(value);
            static_assert(sizeof(bits) == sizeof(number));
            std::memcpy(&bits, &number, sizeof(bits));
            write_le<uint64_t>(out, bits);
        }
        else if (value_is<bool>(value))
        {
            write_le<uint8_t>(out, static_cast<uint8_t>(ConstantTag::Boolean));
            write_le<uint8_t>(out, value_get<bool>(value) ? 1 : 0);
        }
        else if (value_is<std::string>(value))
        {
            write_le<uint8_t>(out, static_cast<uint8_t>(ConstantTag::String));
            write_string(out, value_get<std::string>(value));
        }
        else if (value_is<std::shared_ptr<Function>>(value))
        {
            write_le<uint8_t>(out, static_cast<uint8_t>(ConstantTag::Function));
            write_function(out, *value_get<std::shared_ptr<Function>>(value), depth);
        }
        else if (value_is<IgnoreReturnValue>(value))
            write_le<uint8_t>(out, static_cast<uint8_t>(ConstantTag::Ignore));
        else
            throw std::runtime_error("Unsupported constant in bytecode");
    }
}

Chunk read_chunk(std::istream &in, unsigned depth)
{
    if (depth > max_function_depth)
        throw std::runtime_error("Nested functions exceed the bytecode format limit");
    Chunk chunk;
    const uint32_t code_size = read_le<uint32_t>(in);
    if (code_size > max_code_size)
        throw std::runtime_error("Bytecode chunk exceeds the format limit");
    chunk.code.resize(code_size);
    if (code_size)
        read_raw(in, reinterpret_cast<char *>(chunk.code.data()), code_size);
    const uint32_t constant_count = read_le<uint32_t>(in);
    if (constant_count > max_constants)
        throw std::runtime_error("Bytecode constant table exceeds the format limit");
    chunk.constants.reserve(constant_count);
    for (uint32_t i = 0; i < constant_count; ++i)
    {
        const auto tag = static_cast<ConstantTag>(read_le<uint8_t>(in));
        switch (tag)
        {
        case ConstantTag::Null:
            chunk.constants.emplace_back(nullptr);
            break;
        case ConstantTag::Number:
        {
            const uint64_t bits = read_le<uint64_t>(in);
            double number;
            std::memcpy(&number, &bits, sizeof(number));
            chunk.constants.emplace_back(number);
            break;
        }
        case ConstantTag::Boolean:
        {
            const auto value = read_le<uint8_t>(in);
            if (value > 1)
                throw std::runtime_error("Invalid boolean constant in bytecode");
            chunk.constants.emplace_back(value != 0);
            break;
        }
        case ConstantTag::String:
        {
            std::string value = read_string(in);
            if (!chunk.string_constant_indices.contains(value))
                chunk.string_constant_indices.emplace(value, static_cast<uint16_t>(i));
            chunk.constants.emplace_back(std::move(value));
            break;
        }
        case ConstantTag::Function:
            chunk.constants.emplace_back(read_function(in, depth));
            break;
        case ConstantTag::Ignore:
            chunk.constants.emplace_back(IgnoreReturnValue{});
            break;
        default:
            throw std::runtime_error("Unknown constant type in bytecode");
        }
    }
    return chunk;
}
}

void write_bytecode(std::ostream &output, const Chunk &chunk)
{
    write_raw(output, magic, sizeof(magic));
    write_le<uint16_t>(output, format_version);
    write_chunk(output, chunk, 0);
}

Chunk read_bytecode(std::istream &input)
{
    char file_magic[sizeof(magic)];
    read_raw(input, file_magic, sizeof(file_magic));
    if (std::memcmp(file_magic, magic, sizeof(magic)) != 0)
        throw std::runtime_error("Not a Carblang bytecode file");
    const uint16_t version = read_le<uint16_t>(input);
    if (version != format_version)
        throw std::runtime_error("Unsupported Carblang bytecode version");
    Chunk chunk = read_chunk(input, 0);
    if (input.peek() != std::char_traits<char>::eof())
        throw std::runtime_error("Unexpected trailing data in bytecode file");
    if (input.bad())
        throw std::runtime_error("Failed reading bytecode file");
    return chunk;
}
