# The POST body to test
BODY="123456789"

# Common PATH_INFO guesses
paths=(
    "/"
    "/test"
    "/something"
    "/directory"
    "/youpi.bla"
    "/directory/youpi.bla"
    "/cgi-bin/youpi.bla"
)

for p in "${paths[@]}"; do
    echo "=== Testing PATH_INFO='$p' ==="
    
    export REQUEST_METHOD=POST
    export SERVER_PROTOCOL=HTTP/1.1
    export CONTENT_LENGTH=${#BODY}
    export CONTENT_TYPE=application/octet-stream
    export SCRIPT_NAME="/directory/youpi.bla"
    export PATH_INFO="$p"
    export QUERY_STRING=""
    export REMOTE_ADDR="127.0.0.1"
    export REMOTE_PORT="12345"
    export SERVER_NAME="localhost"
    export SERVER_PORT="8080"
    
    # Run the CGI with the body
    echo -n "$BODY" | strace -f -e trace=file,read,write ./YoupiBanane/ubuntu_cgi_tester
    
    echo -e "\n==============================\n"
done

