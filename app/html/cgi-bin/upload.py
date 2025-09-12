#!/usr/bin/env python3
import os
import sys
import warnings

with warnings.catch_warnings():
    warnings.filterwarnings("ignore", category=DeprecationWarning)
    import cgi



method = os.environ.get("REQUEST_METHOD", "")
content_length = os.environ.get("CONTENT_LENGTH")
content_type = os.environ.get("CONTENT_TYPE")
upload_store = os.environ.get("UPLOAD_STORE")

if method != "POST":
    print("Status: 405 Method Not Allowed")
    print("Content-Type: text/html\n")
    print("<h1>405 Method Not Allowed</h1>")
    sys.exit(0)

if not content_length:
    print("Status: 400 Bad Request")
    print("Content-Type: text/html\n")
    print("<h1>400 Bad Request: CONTENT_LENGTH missing</h1>")
    sys.exit(0)

try:
    body_length = int(content_length)
    if body_length <= 0:
        raise ValueError
except ValueError:
    print("Status: 400 Bad Request")
    print("Content-Type: text/html\n")
    print("<h1>400 Bad Request: CONTENT_LENGTH missing</h1>")
    sys.exit(0)

if not upload_store:
    print("Status: 400 Bad Request")
    print("Content-Type: text/html\n")
    print("<h1>400 Bad Request: UPLOAD_STORE missing</h1>")
    sys.exit(0)

if not content_type or "boundary=" not in content_type:
    print("Status: 400 Bad Request")
    print("Content-Type: text/html\n")
    print("<h1>400 Bad Request: missing boundary</h1>")
    sys.exit(0)

form = cgi.FieldStorage()
fileitem = form['file']

if fileitem.filename:
    filepath = os.path.join(upload_store, os.path.basename(fileitem.filename))
    with open(filepath, 'wb') as f:
        f.write(fileitem.file.read())

print("Status: 201 Created")
print("Content-Type: text/html")
print()
print(f"<h1>uploaded successfully</h1>")

