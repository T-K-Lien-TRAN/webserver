#!/bin/bash
HOST=$1
PORT=$2
PATH=${3:-/}

#GET
/usr/bin/curl -v http://localhost:$PORT$PATH -H "Host: $HOST"

#POST
/usr/bin/curl --data-binary "@42.txt" http://localhost:8081/site4/upload/42.tx -H "Host: $HOST"

#DELETE
/usr/bin/curl -v -X DELETE http://localhost:$PORT$PATH_PARAM -H "Host: $HOST"

#GET SITE
curl -v http://localhost:8080/
curl -v http://localhost:8081/
curl -v -H "Host: virtual_host" http://localhost:8081/
curl -v --resolve virtual_host:8081:127.0.0.1 http://virtual_host:8081/
curl -v --resolve example.com:8082:127.0.0.1 http://example.com:8082/

#ERROR PAGE
curl -v http://localhost:8080/default-error-page.html
curl -v http://localhost:8080/notfound/custom-error-page.html

#BODY LIMIT
curl -v -X POST -H "Content-Type: text/plain" --data "small" http://localhost:8080/bodylimit/
curl -v -X POST -H "Content-Type: text/plain" --data "bigger than 10 bytes" http://localhost:8080/bodylimit/
curl -v -X POST -H "Content-Type: text/plain" --data "1234567890" http://localhost:8080/bodylimit/
curl -v -X POST -H "Content-Type: text/plain" --data "12345678901" http://localhost:8080/bodylimit/

#ROUTING
curl -v http://localhost:8080/about/
curl -v http://localhost:8080/upload/

#METHODS
curl -X DELETE -v http://localhost:8080/methods/somefile.txt
curl -X POST -v http://localhost:8080/methods/
curl -X GET -v http://localhost:8080/methods/

curl -X POST -v --data"Welcome to webserv uplod root" http://localhost:8080/uploads/hello.txt
curl -X POST -v --data-binary "@dog-photo.jpg" http://localhost:8080/uploads/dog-photo.jpg
