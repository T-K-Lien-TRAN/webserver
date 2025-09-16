#GET SITE
curl -v http://localhost:8080/
curl -v http://localhost:8081/
curl -v --resolve virtual_host:8081:127.0.0.1 http://localhost:8081/

#ERROR PAGE
curl -v http://localhost:8080/default-error-page.html
curl -v http://localhost:8080/notfound/custom-error-page.html

#BODY LIMIT
curl -v -X POST -H "Content-Type: text/plain" --data "small" http://localhost:8080/bodylimit/
curl -v -X POST -H "Content-Type: text/plain" --data "bigger than 10 bytes" http://localhost:8080/bodylimit/

#ROUTING
curl -v http://localhost:8080/about/
curl -v http://localhost:8080/upload/
curl -v http://localhost:8080/route/blog/100
curl -v http://localhost:8080/route/blog/42


#METHODS
curl -X DELETE -v http://localhost:8080/methods/somefile.txt
curl -X POST -v http://localhost:8080/methods/
curl -X GET -v http://localhost:8080/methods/
curl -X DELETE -v http://localhost:8080/uploads/dog-photo.jpg
curl -X UNKNOWN -v http://localhost:8080/

#POST
curl -v -F "file=@files_upload/dog-photo.jpg" http://localhost:8080/uploads/upload.py

#DELETE
curl -X DELETE -v http://localhost:8080/upload/dog-photo.jpg

#CGI
curl -v  http://localhost:8080/cgi-bin/$script
curl -v "http://localhost:8080/cgi-bin/hello.py?name=bde-albu"

#BAD_CGI
curl -v http://localhost:8080/cgi-bin/infinite_loop.py

siege -d 1 -c 10 -r 100 http://localhost:8080/siege/index.html



