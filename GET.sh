#!/bin/bash
HOST=$1
PORT=$2
PATH=${3:-/} 
/usr/bin/curl -v http://localhost:$PORT$PATH -H "Host: $HOST"

echo -n
