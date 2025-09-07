#!/bin/bash

export PATH_INFO="/example/path"
export REQUEST_METHOD="POST"
export QUERY_STRING=""
export CONTENT_TYPE=""
export CONTENT_LENGTH="0"   # Important to avoid reading stdin
export SCRIPT_NAME=""
export SERVER_NAME="localhost"
export SERVER_PORT="80"
export REMOTE_ADDR="127.0.0.1"
export REMOTE_PORT="12345"
export SERVER_PROTOCOL="HTTP/1.1"

echo "Content-Type: text/plain; charset=utf-8"
echo ""

# Print all important CGI environment variables
echo "===== CGI ENVIRONMENT VARIABLES ====="
echo "PATH_INFO: $PATH_INFO"
echo "REQUEST_METHOD: $REQUEST_METHOD"
echo "QUERY_STRING: $QUERY_STRING"
echo "CONTENT_TYPE: $CONTENT_TYPE"
echo "CONTENT_LENGTH: $CONTENT_LENGTH"
echo "SCRIPT_NAME: $SCRIPT_NAME"
echo "SERVER_NAME: $SERVER_NAME"
echo "SERVER_PORT: $SERVER_PORT"
echo "REMOTE_ADDR: $REMOTE_ADDR"
echo "REMOTE_PORT: $REMOTE_PORT"
echo "SERVER_PROTOCOL: $SERVER_PROTOCOL"
echo "===================================="

echo "===== CGI ENVIRONMENT VARIABLES ====="
env | grep -E "PATH_INFO|REQUEST_METHOD|QUERY_STRING|CONTENT_TYPE|CONTENT_LENGTH|SCRIPT_NAME|SERVER_NAME|SERVER_PORT|REMOTE_ADDR|REMOTE_PORT|SERVER_PROTOCOL"
echo "====================================="

# Run binary, redirect stdin from /dev/null to avoid hanging
./ubuntu_cgi_tester < /dev/null
