import argparse
import glob
import shutil
import socket
import subprocess
import sys
import tempfile
from pathlib import Path

MAX_UPLOAD_SIZE = 24 * 1024
DEFAULT_PORT = 3333


def parse_endpoint(endpoint, port_override):
    host = endpoint
    embedded_port = None
    if endpoint.startswith("["):
        closing = endpoint.find("]")
        if closing < 0:
            raise ValueError("IPv6 endpoints must use [address] or [address]:port")
        host = endpoint[1:closing]
        suffix = endpoint[closing + 1 :]
        if suffix:
            if not suffix.startswith(":") or not suffix[1:].isdigit():
                raise ValueError("Invalid port in endpoint")
            embedded_port = int(suffix[1:])
    elif endpoint.count(":") == 1:
        possible_host, possible_port = endpoint.rsplit(":", 1)
        if possible_port.isdigit():
            host, embedded_port = possible_host, int(possible_port)

    port = port_override if port_override is not None else embedded_port or DEFAULT_PORT
    if not host:
        raise ValueError("Host cannot be empty")
    if not 1 <= port <= 65535:
        raise ValueError("Port must be between 1 and 65535")
    return host, port


def expand_inputs(specifications, recursive):
    files = []
    seen = set()
    errors = []
    for specification in specifications:
        matches = glob.glob(str(specification), recursive=recursive)
        if not matches:
            candidate = Path(specification)
            if candidate.exists():
                matches = [str(candidate)]
            else:
                errors.append(f"No file or pattern match: {specification}")
                continue

        for match in matches:
            path = Path(match)
            if path.is_dir():
                iterator = path.rglob("*") if recursive else path.iterdir()
                matches_in_directory = sorted(
                    entry
                    for entry in iterator
                    if entry.is_file() and entry.suffix.lower() in (".crb", ".crbc")
                )
                if not matches_in_directory:
                    errors.append(f"No .crb or .crbc scripts in directory: {path}")
                for entry in matches_in_directory:
                    resolved = entry.resolve()
                    if resolved not in seen:
                        files.append(resolved)
                        seen.add(resolved)
            elif path.is_file():
                resolved = path.resolve()
                if path.suffix.lower() not in (".crb", ".crbc"):
                    errors.append(f"Unsupported file type: {path} (use .crb or .crbc)")
                elif resolved not in seen:
                    files.append(resolved)
                    seen.add(resolved)
            else:
                errors.append(f"Not a regular file: {path}")
    return files, errors


def find_compiler(name):
    compiler = shutil.which(name)
    if compiler is None and Path(name).is_file():
        compiler = str(Path(name).resolve())
    return compiler


def compile_or_read(path, compiler):
    if path.suffix.lower() == ".crbc":
        return path.read_bytes()

    with tempfile.TemporaryDirectory(prefix="carblang-") as temporary_directory:
        bytecode_path = Path(temporary_directory) / "upload.crbc"
        compiled = subprocess.run(
            [compiler, str(path), "-o", str(bytecode_path)],
            capture_output=True,
            text=True,
        )
        if compiled.stdout:
            print(compiled.stdout, end="")
        if compiled.returncode != 0:
            if compiled.stderr:
                print(compiled.stderr, file=sys.stderr, end="")
            raise RuntimeError(f"compiler exited with code {compiled.returncode}")
        if compiled.stderr:
            print(compiled.stderr, file=sys.stderr, end="")
        return bytecode_path.read_bytes()


def read_response(connection):
    response = bytearray()
    while len(response) < 256:
        chunk = connection.recv(256 - len(response))
        if not chunk:
            break
        response.extend(chunk)
        if b"\n" in chunk:
            break
    return response.decode("utf-8", errors="replace").strip()


def upload(host, port, token, payload, timeout):
    with socket.create_connection((host, port), timeout=min(timeout, 10)) as connection:
        connection.settimeout(timeout)
        header = f"CRBC1 {token} {len(payload)}\n".encode("ascii")
        connection.sendall(header)
        connection.sendall(payload)
        response = read_response(connection)
    return response, response.startswith("OK ")


def main():
    parser = argparse.ArgumentParser(
        description=__doc__,
        epilog=(
            "Examples:\n"
            "  python send_script.py 192.168.1.7:3333 TOKEN hello.crb other.crbc\n"
            "  python send_script.py 192.168.1.7 TOKEN scripts/ --recursive\n"
            "  python send_script.py 192.168.1.7 TOKEN '*.crb'"
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("host", help="ESP32 IP/hostname, optionally with :port")
    parser.add_argument("token", help="Upload token configured in ESP-IDF menuconfig")
    parser.add_argument(
        "scripts",
        nargs="+",
        metavar="SCRIPT_OR_DIR",
        help="One or more .crb/.crbc files, wildcard patterns, or directories",
    )
    parser.add_argument(
        "--compiler",
        default="small_carblangc",
        help="Compiler executable used for .crb files (default: small_carblangc)",
    )
    parser.add_argument(
        "--port", type=int, help="Override endpoint port (default: 3333)"
    )
    parser.add_argument(
        "--recursive",
        "-r",
        action="store_true",
        help="Search directories recursively (wildcards can use **)",
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=120,
        help="Per-upload response timeout in seconds (default: 120)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Compile and validate scripts without connecting",
    )
    parser.add_argument(
        "--stop-on-error",
        action="store_true",
        help="Stop after the first script that fails",
    )
    args = parser.parse_args()

    if any(character.isspace() for character in args.token):
        parser.error("the upload token must not contain whitespace")
    if not 16 <= len(args.token) <= 95:
        parser.error("the upload token must be between 16 and 95 characters")
    if args.timeout <= 0:
        parser.error("--timeout must be greater than zero")
    try:
        host, port = parse_endpoint(args.host, args.port)
    except ValueError as error:
        parser.error(str(error))

    scripts, input_errors = expand_inputs(args.scripts, args.recursive)
    for error in input_errors:
        print(f"Input error: {error}", file=sys.stderr)
    if not scripts:
        return 2

    compiler = None
    if any(path.suffix.lower() == ".crb" for path in scripts):
        compiler = find_compiler(args.compiler)
        if compiler is None:
            print(
                "small_carblangc was not found; install it or pass its path with --compiler",
                file=sys.stderr,
            )
            return 2

    failures = len(input_errors)
    successes = 0
    for index, path in enumerate(scripts, start=1):
        print(f"[{index}/{len(scripts)}] {path.name}")
        try:
            payload = compile_or_read(path, compiler)
            if not payload.startswith(b"CRBC"):
                raise ValueError("not a Carblang .crbc bytecode file")
            if len(payload) > MAX_UPLOAD_SIZE:
                raise ValueError(
                    f"bytecode is {len(payload)} bytes; ESP32 limit is {MAX_UPLOAD_SIZE}"
                )
            if len(payload) < 6:
                raise ValueError("bytecode file is too short")
            if args.dry_run:
                print(f"  Valid bytecode ({len(payload)} bytes); dry run, not sent")
                successes += 1
                continue

            response, succeeded = upload(host, port, args.token, payload, args.timeout)
            if response:
                print(f"  {response}")
            else:
                print(
                    "  Upload failed: ESP32 closed without a response", file=sys.stderr
                )
            if not succeeded:
                failures += 1
                if args.stop_on_error:
                    break
            else:
                successes += 1
        except (OSError, UnicodeEncodeError, ValueError, RuntimeError) as error:
            print(f"  Failed: {error}", file=sys.stderr)
            failures += 1
            if args.stop_on_error:
                break

    print(f"Finished: {successes} succeeded, {failures} failed.")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
