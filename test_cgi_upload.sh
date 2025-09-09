#!/bin/bash
export REQUEST_METHOD=POST
export CONTENT_LENGTH=29
export PATH_INFO=/uploads/hello.txt
export UPLOAD_STORE=www/
echo "Welcome to webserv uplod root" | ./cgi_upload