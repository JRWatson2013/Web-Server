/* client.c: http client */
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
#include <time.h>

int main(int argc, char *argv[])
{
  int sockfd; /* socket file discriptor */
  struct addrinfo hints, *servinfo, *p; /* used by getaddrinfo */
  int rv, bytes; /*rv is the setup checksum, and bytes holds the number of bytes in the return message */
  char message[10000000]; /*holds the ingoing/outgoing message*/
  char host[100]; /*used to hold the parced file location*/
  char url[100]; /*used to hold the parced host address*/
  int pval; /*used to determine if the '-p' option is set*/
  char pvalCheck[3]; /* used to check is the option is set to -p */
  struct timeval start, end; /* used to measure the RTT */
  char htmlCheck[7]; /*contains '/'. used to locate the file location within the input*/
  char *urlParcer; /*used in message transmission while loop to see if message is done transmitting */
  char input[500]; /* holds the destination input from the command line before it is processed */
  int hostSize; /* holds the size of the host section of the input. used when parcing the host. */
  float RTT; /* holds the calculated RTT */
  float startingCalc; /* used to calculate the RTT */
  float endingCalc; /* used to calculate the RTT */

  /*start the program */

  sprintf(htmlCheck, "/"); /* sets htmlCheck, so it can be used to parce out the file location later */
  /*  printf("\n %s \n", htmlCheck); DEBUG!! */ 
  sprintf(pvalCheck, "-p"); /* sets pvalCheck, so it can be compared to check for the -p option */
  pval = 1; /* set pval to false */
  if (argc > 4 || argc < 3) /*have the proper number of commands have been entered?*/
    {    
   fprintf(stderr, "usage: ~~ [options] URL/IP PORT");
   return (2);
    }

  /*  printf("\n argv[1]: %s \n argv[2]: %s", argv[1], argv[2]); DEBUG!! */

  memset(&hints, 0, sizeof hints); /* make sure nothing is in hints */
  hints.ai_family = AF_UNSPEC; /* using either IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM; /* using TCP */

  if (argc == 3) /* if no option has been entered on the command line */
   {
     strcpy(input ,argv[1]); /* pull the address from the command line and store it in a string */
     urlParcer  = strstr(input, "/"); /* find the start of the file location within the address */

     if(urlParcer != NULL)  /* if no file location is included, assume the user meant the location '/', and set the file location to that, otherwise parce out the file location */
       {
	 strcpy(url, urlParcer); /*store the file location in url */
       }
     else
       {
	 sprintf(url, "/"); /* store the default file location in URL */
       }
     if(urlParcer != NULL) /* if the file location isnt the default, find the length of the address minus the file location */ 
       {
	 hostSize = ((strlen(input) - ((strlen(url))))); /* length of address - length of file location = length of host */
       }
     else
       {
	 hostSize = strlen(input); /* if the file location is default, then the host size is the same as the address */
       }
     strncpy(host, input, hostSize); /* parce the host out of the address */
     /*  printf("\n URL: %s \n HOST: %s \n", url, host); DEBUG */
     if ((rv = getaddrinfo(host, argv[2], &hints, &servinfo)) != 0){ /* get dest info */
	   fprintf(stderr, "error getting address info \n program will terminate \n");
	   return 1;
     }
   }
  else if (argc == 4) /*if a option was entered on the command line */
   {
     if((strncmp(argv[1], pvalCheck, 3)) == 0) /* make sure that option is '-p' */
       {
	 pval = 0; /* set pval to true */
       }
     strcpy(input ,argv[2]); /*same as in argc ==3 */
     urlParcer  = strstr(input, "/"); /* same as in argc == 3 */
     if(urlParcer != NULL) /* same as in argc == 3 */
       {
	 strcpy(url, urlParcer); /* same as in argc == 3 */
       }
     else
       {
         sprintf(url, "/"); /* same as in argc == 3 */
       }
     if(urlParcer != NULL) /* same as in argc == 3 */
       {
	 hostSize = ((strlen(input) - ((strlen(url)))));
       }
     else
       {
         hostSize = strlen(input); /* same as in argc == 3 */
       }
     strncpy(host, input, hostSize); /* same as in argc == 3 */
     /*  printf("\n URL: %s \n HOST: %s \n", url, host); DEBUG!! */

     if ((rv = getaddrinfo(host, argv[3], &hints, &servinfo) != 0)){ /* get dest info, different than argc == 3 in that argv[3] is used instead of argv[2] for the port */
       fprintf(stderr, "error getting address info \n program will terminate \n");
       return 1;
     }
   }
  for(p = servinfo; p != NULL; p = p->ai_next) /* cycle through the list of potential hosts, and try to create a socket/connect */
   {
     if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) /* attempt to create a socket */
       {
	 fprintf(stderr, "error creating socket. \n");
	   continue;
       }
     gettimeofday(&start, 0); /* start RTT */
     if(connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)  /* attempt to connect to host */
       {
	 fprintf(stderr, "error connecting to host. \n");
	 /*  printf("%d\n%d\n%d\n", &p, &servinfo, &p->ai_next); */
	 close(sockfd);
	 continue;
       }
     break;
   }

  /*  printf("\n aifam: %d \n socktype: %d \n  protocol: %d", p->ai_family, p->ai_socktype, p->ai_protocol); DEBUG!! */ 


  /*  printf("\n sockfd: %d \n", sockfd); DEBUG!! */ 

  if(argc == 4) /* set the message field to the GET request */
   {
     sprintf(message, "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", url, host);
   }
  if(argc == 3) /* set the message field to the GET request */
   {
     sprintf(message, "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", url, host);
   }
  if (p == NULL) /* if a connection could not be made, terminate the program */
   {
     fprintf(stderr, "A connection could not be established to the supplied server. \n");
     return 2;
   }

  /* printf("\n Message: %s \n", message); DEBUG!! */ 



  if(send(sockfd, message, strlen(message), 0) == -1) /* attempt to send the GET request, if it fails, end the program */
   {
     fprintf(stderr, "error sending message. \n The program will terminate.");
     close(sockfd);
     return 2;
   }


  bzero(message, 10000000); /* blank out the message field to prepare to store the response message */

 bytes = recv(sockfd, message, 10000000, 0); /* receive the message, store the size in bytes */
 /*  messageDone = strstr(message, htmlCheck); DEBUG!!  */ 

 gettimeofday(&end, 0); /*stop counting RTT */

 endingCalc = (end.tv_sec * 1000) - (start.tv_sec * 1000) + ((end.tv_usec / 1000) - (start.tv_usec / 1000)); /* calculate the RTT */

 RTT =  endingCalc; /* move the result into the RTT field for clarity */

 printf("Message recieved from %s : \n %s \n Number of bytes is %d \n", host, message, bytes); /* Print out the servers response and the size of the message */
 if(pval == 0) /* if the RTT was requested, print it out. */
   printf("The RTT was %f. \n", RTT );

 close(sockfd); /* close the socket */

 freeaddrinfo(servinfo); /* get rid of the address linked list */


 return 0;
}
