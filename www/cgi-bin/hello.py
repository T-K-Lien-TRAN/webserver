#!/usr/bin/env python3

import os
import sys
import cgi

# Print HTTP headers
print("Content-Type: text/html")
print()  # blank line separates headers from body

# Start HTML output
print("<html><body>")
print("<h1>Hello from Python CGI!</h1>")

# Show some environment info
print("<h2>Environment Variables:</h2>")
print("<ul>")
for key in ["REQUEST_METHOD", "QUERY_STRING", "CONTENT_LENGTH", "CONTENT_TYPE", "PATH_INFO"]:
    value = os.environ.get(key, "")
    print(f"<li>{key}: {value}</li>")
print("</ul>")

# If POST, read the body
if os.environ.get("REQUEST_METHOD") == "POST":
    try:
        content_length = int(os.environ.get("CONTENT_LENGTH", 0))
    except ValueError:
        content_length = 0
    if content_length > 0:
        post_body = sys.stdin.read(content_length)
        print("<h2>POST Body:</h2>")
        print(f"<pre>{post_body}</pre>")

print("</body></html>")


