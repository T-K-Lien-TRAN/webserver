#!/bin/bash
HOST=$1
PORT=$2
PATH_PARAM=${3:-/}

# Send DELETE request
/usr/bin/curl -v -X DELETE http://localhost:$PORT$PATH_PARAM \
  -H "Host: $HOST"
