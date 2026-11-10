import argparse
import shutil
import socket
import subprocess
import sys
import tempfile
from pathlib import Path

MAX_UPLOAD_SIZE = 24 * 1024


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "host", help='ESP32 IP address shown by the "wifi" shell command'
    )
    parser.add_argument("token", help="Upload token configured in ESP-IDF menuconfig")
    parser.add_argument("script", type=Path, help="Source .crb or compiled .crbc file")
    parser.add_argument(
        "--compiler",
        default="small_carblangc",
        help="Compiler executable for .crb input",
    )
    parser.add_argument("--port", type=int, default=3333)
    args = parser.parse_args()

    try:
        if args.script.suffix.lower() == ".crb":
            compiler = shutil.which(args.compiler)
            if compiler is None and Path(args.compiler).is_file():
                compiler = str(Path(args.compiler).resolve())
            if compiler is None:
                print(
                    "small_carblangc was not found, pass its path with --compiler",
                    file=sys.stderr,
                )
                return 2
            with tempfile.TemporaryDirectory(prefix="carblang-") as temporary_directory:
                bytecode_path = Path(temporary_directory) / "upload.crbc"
                compiled = subprocess.run(
                    [compiler, str(args.script), "-o", str(bytecode_path)],
                    capture_output=True,
                    text=True,
                )
                if compiled.returncode != 0:
                    print(compiled.stdout + compiled.stderr, file=sys.stderr, end="")
                    return compiled.returncode
                payload = bytecode_path.read_bytes()
        elif args.script.suffix.lower() == ".crbc":
            payload = args.script.read_bytes()
        else:
            print("Input must have a .crb or .crbc extension", file=sys.stderr)
            return 2
    except OSError as error:
        print(f"Cannot read or compile script: {error}", file=sys.stderr)
        return 2

    if not payload.startswith(b"CRBC"):
        print("Compiler output is not a Carblang .crbc bytecode file", file=sys.stderr)
        return 2
    if len(payload) > MAX_UPLOAD_SIZE:
        print(
            f"Bytecode exceeds the ESP32 upload limit of {MAX_UPLOAD_SIZE} bytes",
            file=sys.stderr,
        )
        return 2
    if any(character.isspace() for character in args.token):
        print("The upload token must not contain whitespace", file=sys.stderr)
        return 2
    if not 16 <= len(args.token) <= 95:
        print("The upload token must be between 16 and 95 characters", file=sys.stderr)
        return 2

    try:
        with socket.create_connection((args.host, args.port), timeout=10) as connection:
            connection.settimeout(120)
            connection.sendall(f"CRBC1 {args.token} {len(payload)}\n".encode("ascii"))
            connection.sendall(payload)
            response = connection.recv(256).decode("utf-8", errors="replace").strip()
            print(response)
            return 0 if response.startswith("OK ") else 1
    except (OSError, UnicodeEncodeError) as error:
        print(f"Upload failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
