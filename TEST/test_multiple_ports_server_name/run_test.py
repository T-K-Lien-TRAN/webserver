import http.client

tests = [
    ("example1.com", 8080, "TEST 1"),
    ("example2.com", 8080, "TEST 2"),
    ("example3.com", 8000, "TEST 3"),
    ("example4.com", 8000, "TEST 4"),
]

def run_test(host, port, expected):
    conn = http.client.HTTPConnection("127.0.0.1", port, timeout=2)
    headers = {"Host": host}
    try:
        conn.request("GET", "/", headers=headers)
        response = conn.getresponse()
        body = response.read().decode()
        print(f"    Expected to find: {expected}")
        print(f"    Got: {body}")
    finally:
        conn.close()

for host, port, expected in tests:
    run_test(host, port, expected)
