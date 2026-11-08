import os

EXTENSIONS = {".cpp", ".hpp", ".tpp", ".crb"}


def count_lines_in_file(file_path):
    total = 0
    try:
        with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
            for line in f:
                total += 1
    except Exception as e:
        print(f"Error reading {file_path}: {e}")
    return total


def count_lines(folder):
    total_lines = 0
    file_counts = []

    for root, _, files in os.walk(folder):
        for file in files:
            ext = os.path.splitext(file)[1]
            if ext in EXTENSIONS:
                path = os.path.join(root, file)
                lines = count_lines_in_file(path)

                file_counts.append((path, lines))
                total_lines += lines

    return total_lines, file_counts


if __name__ == "__main__":
    folder = "."

    total, details = count_lines(folder)

    for path, lines in details:
        print(f"{path}: {lines} lines")

    print(f"{total} lines")
