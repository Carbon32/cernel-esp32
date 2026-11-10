import os
import subprocess

folder = "crb"

passed = 0
failed = 0

for root, dirs, files in os.walk(folder):
    for filename in files:
        if filename.endswith(".crbc"):
            filepath = os.path.join(root, filename)

            print(f"Testing: {filepath}", end=" -> ")

            try:
                run_input = "Elise\n26\n" if filepath.endswith("scan.crbc") else None

                result = subprocess.run(
                    ["small_carblang", str(filepath)],
                    input=run_input,
                    capture_output=True,
                    text=True,
                )

                output = result.stdout + result.stderr

                if output.strip().endswith("valid execution"):
                    print("[PASS]")
                    passed += 1
                else:
                    print("[FAIL]")
                    print("Output:")
                    print(output)
                    failed += 1

            except FileNotFoundError:
                print(
                    'Error: "small_carblangc" command not found, make sure you compiled the project first.'
                )
                exit(1)

print(f"Passed: {passed}")
print(f"Failed: {failed}")

total = passed + failed
print(f"Total:  {total}")

if total != 27:
    print(f"There should be 27 compiled files, but for some reason there are {total}")
