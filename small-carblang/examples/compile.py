import os
import subprocess

folder = "crb"

for root, dirs, files in os.walk(folder):
    for filename in files:
        if filename.endswith(".crb"):
            input_path = os.path.join(root, filename)
            output_path = os.path.join(root, filename + "c")

            print(f"Compiling: {input_path}")

            try:
                subprocess.run(
                    ["small_carblangc", input_path, "-o", output_path], check=True
                )
            except subprocess.CalledProcessError as e:
                print(f"Failed to compile {input_path}: {e}")
            except FileNotFoundError:
                print(
                    'Error: "small_carblangc" command not found, make sure you compiled the project first.'
                )
                exit(1)

print("Compilation complete")
