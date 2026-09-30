import serial, sys, time
port = sys.argv[1] if len(sys.argv) > 1 else "COM14"
duration = float(sys.argv[2]) if len(sys.argv) > 2 else 12.0
try:
    s = serial.Serial(port, 115200, timeout=0.2, dsrdtr=False, rtscts=False)
except Exception as e:
    print(f"OPEN_FAIL: {e}")
    sys.exit(1)
# Reset the ESP32 by toggling DTR (EN pin)
s.setDTR(False)
s.setRTS(True)
time.sleep(0.1)
s.setRTS(False)
time.sleep(0.05)
s.setDTR(False)
deadline = time.time() + duration
while time.time() < deadline:
    data = s.read(4096)
    if data:
        try:
            sys.stdout.write(data.decode('utf-8', errors='replace'))
        except Exception:
            sys.stdout.buffer.write(data)
        sys.stdout.flush()
s.close()
