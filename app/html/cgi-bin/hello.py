#!/usr/bin/env python3
import cgi, cgitb
cgitb.enable()

print("Content-type: text/html\n")
print("<html>")
print("<head><title>CGI Example</title></head>")
print("<body>")
print("<h1>Hello from CGI!</h1>")

form = cgi.FieldStorage()
name = form.getvalue("name", "Guest")
print(f"<p>Hello, {name}!</p>")
print("</body>")
print("</html>")
