/* server.c: Server program */
/* Jacob Watson */
/* JRWatson */
/* JRWatson@wpi.edu */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>


/* removes zombie processes */
void sigchld_handler (int s)
{
  while(waitpid(-1, NULL, WNOHANG) > 0);
}

/* get sockaddr, IPv4 or IPv6 */
void *get_in_addr(struct sockaddr *sa)
{
  if(sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int main(int argc, const char*  argv[])
{
  struct addrinfo hints, *servinfo, *p; /* used to setup server */
  int sockfd; /* socket file discriptor */
  int sock_in; /*incoming socket file discriptor */
  int setupStat; /* used to check if servinfo was created properly */
  int bindCheck; /* used to check if the port bind worked correctly */
  struct sockaddr_storage incoming; /* holds the address info for the connecting machine */
  socklen_t incomingSize; /* size of incoming packet */
  struct sigaction sa; /* used to handle zombie processes */
  char s[INET6_ADDRSTRLEN]; /* used to print out the connecting clients IP */
  char in_message[100000]; /*holds GET request from client */
  char http200[500]; /* holds the 200 OK header */
  char getString[7]; /* holds 'GET' to compare to incoming request */
  char findHTTP[10]; /* used to parse the file location. contains 'HTTP/1.1' */
  char http400[500]; /* holds the 'bad request message */
  char http404[500]; /* holds the 'file not found' message */
  char *endRequest; /* marks the end of the file location */
  int stringToRemove; /* used to find where file location parcing should end */
  char firstLocParce[50]; /* holds the first-run parce of the file location */
  char secondLocParce[50]; /* holds the second-run parce of the file location */
  FILE *requested; /* will point to requested file */
  char rdin; /* used to transfer the requested file to the message */
  char *file_contents; /* points to the memory for storing the file contents*/
  long file_size; /* holds the file size */
  char* out_message; /* points to the memory for the out message */
  long message_size; /* holds the out  message size */
  char* remvSpace; /* points to the first space in the file location, used to parse out the file location */
  char spc[7]; /* used in parcing out the file location. contains ' ' */
  int yes;  /* used as a place holder for  '1' */


  /* program starts */
  sprintf(findHTTP, "HTTP/1.1"); /* set findHTTP for parcing */
  sprintf(getString, "GET"); /* set getString for parcing */
  sprintf(http400, "HTTP/1.1 400 BAD REQUEST\r\nConnection: close\r\n\r\n"); /* set bad request header */
  sprintf(http404, "HTTP/1.1 404 NOT FOUND\r\nConnection: close\r\n <!DOCTYPE html PUBLIC \"-//IETF//DTD HTML 2.0//EN\"> <HTML> <BODY> 404 Bad Request </BODY> </HTML>\r\n");
 /* set not found header */
  sprintf(http200, "HTTP/1.1 200 OK\r\nConnection: close\r\n"); /* set ok header */
  sprintf(spc, " "); /* set spc to ' ' for parcing */

  memset(&hints, 0 , sizeof hints);  /* zeroes out hints */
  endRequest == NULL; /* set to default */
  requested == NULL; /* set to default */
  file_contents = NULL; /* set to default */
  out_message = NULL; /* set to default */
  remvSpace = NULL; /* set to default */
  yes = 1; /* set to one for use later */
  hints.ai_family = AF_UNSPEC; /* either v4 or v6 is fine */
  hints.ai_socktype = SOCK_STREAM; /* TCP will be used */
  hints.ai_flags = AI_PASSIVE; /* automatically aqquire server IP */

  /* assemble the info needed to run the server through the getaddrinfo method */
  if((setupStat = (getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0))
    {
      fprintf(stderr, "There was a error setting up the server. \n Error: %s \n", gai_strerror(setupStat));
      return 1;
    }

 
  /* attempt to use the servinfo linked list to create a socket */
  for (p =servinfo; p != NULL; p = p->ai_next)
    {
      /* create socket */
      if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
	{
	  perror("There was a issue creating a socket. \n");
	  continue;
	}

      /*set the socket options */
      if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
          perror("There was a issue setting the socket options \n");
	  close(sockfd);
          exit(1);
        }

      /* attempt to bind the socket to the port */
      if((bind(sockfd, p->ai_addr, p->ai_addrlen)) == -1)
	{
	  perror("There was a issue binding the socket to the specified port. \n");
	  close(sockfd);
	  continue;
	}
      break;
    }

  /* check to see if no socket/port combo could be created, and if not, end the program */
  if(p == NULL)
    {
      perror("The server was unable create a socket or bind it to the specified port. \n The program will now end. \n");
      return (2);
    }

  /* free the linked list */
  freeaddrinfo(servinfo); 

  /* start listening for incomming connections */
  if((listen(sockfd, 10) == -1))
    {
      perror("The server was unable to begin listening. \n The program will now end. \n");
      close(sockfd);
      return (2);
    }
     
  /* set up sa to handle and control zombie processes */
     sa.sa_handler = sigchld_handler;
     sigemptyset(&sa.sa_mask);
     sa.sa_flags = SA_RESTART;

     /* if there is a issue with sa, end the program */
     if (sigaction(SIGCHLD, &sa, NULL) == -1)
       {
	 perror("There was a error with sigaction \n The program will end. \n");
	 close(sockfd);
	 exit(1);
       }

     /* Notify user the server is ready */
     printf("The server is now ready to receive requests. \n");

     /* begin servicing incoming requests. Infinite looped in order to continue service until program is killed */

     while(1)
       {
	 /* reset incomingSize to default */
	 incomingSize = sizeof incoming;

	 /* accept incoming connection, and assign it to a new socket */
	 sock_in = accept(sockfd, (struct sockaddr *)&incoming, &incomingSize);

	 /* make sure the new socket was created correctly */	
	 if(sock_in == -1)
	   {
	     perror("There was a error accepting a incoming connection. \n");
	     continue;
	   }

	 /* switch around the ip info for display */
	 inet_ntop(incoming.ss_family, get_in_addr((struct sockaddr *)&incoming), s, sizeof(s));

	 /*inform user that a connection has been made */
	 printf("A connection has been made from: %s \n", s);

	 /* create a child process to handle the sending and receiving */
	 if(!fork())
	   {
	     close(sockfd); /*not needed here, so close it to be safe */
       /*receive the message*/
       recv(sock_in, in_message, 100000, 0);
       /*  printf("%s \n", in_message); DEBUG!! */
       /* check to see if the message is a GET request */
       /* if the message is bad, return a bad request response */
       /* if the first three characters of the message aren't GET, the request will be considered bad */
       if((strspn(in_message, getString)) < 3)
	 {
	   printf("Bad request recieved. \n");
	   send(sock_in, http400, 500, 0);
	   close(sock_in);
	   exit(0);
	 }

       /*  printf("point1\n"); DEBUG!! */

       /*locate the 'HTTP/1.1', so the end of the file location can be found */
       endRequest = strstr(in_message, findHTTP);


       /*   printf("endrequest: %s", endRequest); DEBUG!! */


       /* if there is no 'HTTP/1.1', the request is bad */
       if(endRequest == NULL)
	 {
	   printf("Bad request recieved. No HTTP/1.1 \n");
	   send(sock_in, http400, 500, 0);
	   close(sock_in);
	   exit(0);
	 }

       /*  printf("point2\n"); DEBUG!! */

       /*subtract the size of the 'HTTP/1.1' and every part of the request beyond it to find the length of the first section of the request containing 'GET' and the file location */
       stringToRemove = ((strlen(in_message)) - ((strlen(endRequest))));
       /*if stringToRemove == 4, then there is no file location in the request ('GET ' == 4), and the request is bad */
       if(stringToRemove <= 4)
	 {
	   printf("Bad request recieved. No file was requested. \n");
	   send(sock_in, http400, 500, 0);
	   close(sock_in);
	   exit(0);
	 }
       
       /*   printf("Point Fail \n"); DEBUG!!  */
 
       /* parse out the 'GET [file location] ' from the message, happens over the next few steps */

       /*  printf("first: %s \n in: %s \n string: %d \n", firstLocParce, in_message, stringToRemove); DEBUG!! */

       /*copy out the address from the header */
       strncpy(firstLocParce, in_message, stringToRemove);

       /*  printf("err3\n"); DEBUG!! */

       /* locate the first space after the 'GET' */
       remvSpace = strstr(firstLocParce, spc);

       /*  printf("firstloc: %s\n",firstLocParce); DEBUG!! */

       /* parse out the '[file location]' from the 'GET ' and ' ' */
       strncpy(secondLocParce, (remvSpace + 2), (stringToRemove - 6));

       /*  printf("SecondLoc:%s\n", secondLocParce); DEBUG!! */

       /* check to see if the requested file exists, and if so, get a FILE pointer to it */
       requested = fopen(secondLocParce, "r");

       /*  printf("point4\n"); DEBUG!! */

       /*if requested == NULL, then the file was not found */
       if(requested == NULL)
	 {
	   printf("Requested file was not found.");
	   send(sock_in, http404, 500, 0);
	   close(sock_in);
	   exit(0);
	 }

       /* printf("point5\n"); DEBUG!! */

       /*this section pulls out the file contents, and saves them in file_contents as a string */
       fseek(requested, 0, SEEK_END);
       file_size = ftell(requested);
       rewind(requested);
       file_contents = malloc((file_size + 1) * (sizeof(char)));
       fread(file_contents, sizeof(char), file_size, requested);
       fclose(requested);
       file_contents[file_size] = 0;

       /* printf("point6\n"); DEBUG!! */

       /*find the size of the whole out_message (header + file) */
       message_size = (strlen(http200)) + (file_size + 1);

       /*allocate enough memory in out_message for whole response message */
       out_message = malloc((message_size) * (sizeof(char)));

       /* add the header and file together in out_message */
       sprintf(out_message,"%s%s\r\n", http200, file_contents);

       /* let the user know that a response is being sent */
       printf("Sending response...\n");

       /*attempt to send the message, if it fails, let the user know */
       if(send(sock_in, out_message, message_size, 0) == -1)
	 {
	   perror("Could not send message to client. \n");
	 }

       /*clean up after the sub process */
       close(sock_in);
       free(file_contents);
       file_contents == NULL;
       free(out_message);
       out_message = NULL;
       /*  printf("ending sub\n"); DEBUG!! */
       exit(0);
	   }
	 close(sock_in); /* close the incoming socket */
       }
     return 0; /* here for completion, can't actually be reached by the program */
}
