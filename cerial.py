import serial
import threading
import sys

# This will be re-written and improved later

PORT = "COM3"
BAUD = 115200

ready_to_print = False


def read_from_port(ser):
    global ready_to_print
    while True:
        try:
            line = ser.readline()
            if line:
                try:
                    decoded = line.decode("utf-8", errors="ignore").strip()

                    if "Cernel is running" in decoded:
                        ready_to_print = True

                    if ready_to_print:
                        is_ascii_clean = all(
                            32 <= ord(c) < 126 or c in "\n\r\t" for c in decoded
                        )

                        is_noise = (
                            not decoded
                            or not is_ascii_clean
                            or ("++" in decoded)
                            or (len(decoded) < 3 and not decoded.isdigit())
                        )

                        if not is_noise:
                            print(f"\r[ESP] {decoded}\n> ", end="", flush=True)
                except Exception:
                    pass
        except serial.SerialException:
            print("\n[Disconnected]")
            break


def main():
    try:
        ser = serial.Serial(PORT, BAUD, timeout=0.1)
        print(f"Connected to {PORT} at {BAUD}")

        thread = threading.Thread(target=read_from_port, args=(ser,), daemon=True)
        thread.start()

        while True:
            user_input = sys.stdin.readline()
            if not user_input:
                break

            ser.write(user_input.encode("utf-8"))

            if ready_to_print:
                sys.stdout.write("> ")
                sys.stdout.flush()

    except KeyboardInterrupt:
        print("\nExiting...")
    except Exception as e:
        print(f"Error: {e}")


if __name__ == "__main__":
    main()
