import re
import sys
from pathlib import Path

if(len(sys.argv) != 3):
    print("Usage: python macro_builder.py <file.hpp> <type>")
    sys.exit(1)

path = Path(sys.argv[1])
text = path.read_text()
method_type = sys.argv[2]
case_pattern = re.compile(
    r'case\s+NativeMethod::(\w+)\s*:\s*\{.*?\n\}',
    re.DOTALL
)

def convert(match: re.Match) -> str:
    name = match.group(1)
    block = match.group(0)

    lines = block.splitlines()
    macro_name = f"{method_type}_{name}"

    out = [f"#define {macro_name} \\"]
    for line in lines:
        line = line.rstrip()
        if(line):
            out.append(f"    {line} \\")
        else:
            out.append("    \\")
    out[-1] = out[-1].rstrip(" \\")

    print(f"{macro_name}")
    return "\n".join(out)

new_text, count = case_pattern.subn(convert, text)

if(count == 0):
    print("No NativeMethod cases found. The file will remain unchanged")
    sys.exit(0)

path.write_text(new_text)
print(f"Converted {count} NativeMethod cases into macros")