import socket
import time

def send_raw_request(raw_data, label):
    print(f"\n--- Sending: {label} ---")
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect(('localhost', 8080))
        s.sendall(raw_data.encode())
        try:
            response = s.recv(1024)
            print("Response:", response.decode(errors='ignore'))
        except Exception as e:
            print("Error receiving response:", e)

tests = {
    "Missing CRLF": "GET / HTTP/1.1\nHost: test\nContent-Length: 5\nThisShouldBeBody",
    "Empty Request": "",
}

for label, raw in tests.items():
    send_raw_request(raw, label)
    time.sleep(1);
