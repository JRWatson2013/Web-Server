//JRWatson
//Jacob Watson
//JRWatson@wpi.edu
//Project 1, Computer Networks CS3516

The project is split between two .c files; client.c, and server.c. In order to make the files, type 'make' into the commandline. If this fails, the files can be complied by typing:

gcc server.c -o server
gcc client.c -o client

To use the client, please use the format:
./client [option] [URL] [PORT]
OR
./client [URL] [PORT]

Where option can be: -p (NOT -P). If anything but -p is entered, the client will ignore it and act as if no option was given.
The URL is for the web address you wish to connect to. If the URL is not valid, the client will not be able to connect, so be sure to enter a valid one.
The PORT is the port on that server you wish to connect to. If the port is not valid, the client will not be able to connect, so be sure to enter a valid one.


To use the server, please use the format:
./server [PORT]
Where PORT can be any port you wish to host the server on. If the port is not valid, the server will not set itself up, and issue a error, so be sure to enter a valid one.

The code within these files was inspired by/made from the tutorials at http://beej.us/guide/bgnet/output/html/multipage/index.html. 
The code to read in the requested file from a server was inspired by the suggestions at http://stackoverflow.com/questions/7856741/how-can-i-load-a-whole-file-into-a-string-in-c.
