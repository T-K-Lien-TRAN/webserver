#!/usr/bin/env python3

import cgi
import os
import sys

print("Content-Type: text/plain\n")

try:
    content_length = int(os.environ.get("CONTENT_LENGTH", 0))
    post_data = sys.stdin.read(content_length)
    print("You sent the following data via POST:")
    print(post_data)
except Exception as e:
    print(f"Error reading POST data: {e}")
