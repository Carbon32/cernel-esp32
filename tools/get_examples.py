import argparse
import shutil
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parent
DEFAULT_SOURCE = TOOLS_DIR.parent / "small-carblang" / "examples" / "crb"
# Just don't mess around with paths or you will end up breaking some of the scripts


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--source",
        type=Path,
        default=DEFAULT_SOURCE,
        help="Examples folder to search recursively",
    )
    parser.add_argument(
        "--destination",
        type=Path,
        default=TOOLS_DIR,
        help="Folder to receive the .crbc files (default: this tools folder, literally so you may get a files bomb)",
    )
    args = parser.parse_args()

    source = args.source.resolve()
    destination = args.destination.resolve()
    if not source.is_dir():
        parser.error(f"source folder does not exist: {source}")

    bytecode_files = sorted(source.rglob("*.crbc"))
    if not bytecode_files:
        print(f"No .crbc files found under {source}")
        return 1

    names = [path.name.casefold() for path in bytecode_files]
    if len(names) != len(set(names)):
        print("Cannot flatten examples, duplicate .crbc filenames found.")
        return 1

    destination.mkdir(parents=True, exist_ok=True)
    copied = 0
    for source_file in bytecode_files:
        output_file = destination / source_file.name
        shutil.copy2(source_file, output_file)
        print(f"Copied {source_file.relative_to(source)} -> {output_file}")
        copied += 1

    print(f"Copied {copied} compiled example(s) to {destination}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
