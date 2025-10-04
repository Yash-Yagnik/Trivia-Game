/*** socket/demo2/client.c ***/

/*******************************************************************************
 * Name        : client.c
 * Author      : Yash Yagnik
 ******************************************************************************/

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>

// final 5/2/24 8:30pm

int main(int argc, char* argv[]){
    int    server_fd;
    struct sockaddr_in server_addr;
    socklen_t addr_size = sizeof(server_addr);
    //int i = 0;

    int opt;
    char* question_file = "questions.txt";
    char* IP_address = "127.0.0.1";
    int port_number = 25555;

    

   // In each of the cases you might have to check if the user does not enter a second argument for the specific flag put the case in there if you start running into problems 
    while((opt = getopt(argc, argv, ":fiph")) != -1){
        switch (opt){
            case 'f':
                //printf("Option f was selected\n");
                question_file = argv[2]; // Setting the question file to the users input
                break;
            case 'i':
                //printf("Option i was selected\n");
                IP_address = argv[2]; // Setting the ip addy to the users input
                break;
            case 'p':
                //printf("Option p was selected\n");
                port_number = atoi(argv[2]); // Setting the port number to the users input
                break;
            case 'h':
                printf("Usage: %s [-i IP_address] [-p port_number] [-h]\n\t-f question_file Default to '%s' ;\n\t-i IP_address Default to '%d.%d.%d.%d';\n\t-p port_number Default to 25555;\n\t-h Display this help info.\n", argv[0], "question.txt" , 127 , 0, 0, 1);
                return EXIT_SUCCESS;
                break;
            case '?':
                fprintf(stderr, "Error: Unknown option '%s' received\n", argv[1]); // This is if the user enters an invalid flag
                return EXIT_FAILURE;
                break;
        }

    }

    

    void parse_connect(int argc, char** argv, int* server_fd){
      /* STEP 1:
    Create a socket to talk to the server;
    */
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(port_number);
    server_addr.sin_addr.s_addr = inet_addr(IP_address);

    /* STEP 2:
    Try to connect to the server.
    */
    
    if (connect(server_fd, (struct sockaddr *) &server_addr, addr_size) == -1) {
        perror("Error: Connection failed");
        exit(EXIT_FAILURE);
    }


      
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Error: Socket creation failed");
        exit(EXIT_FAILURE);
    }

    parse_connect(argc, argv, server_fd);

    
    char buffer[1024];

    fd_set myset;
    //FD_SET(server_fd, &myset);
    memset(buffer, 0, 1024);
    while(1) {
        FD_SET(server_fd, &myset);
        FD_SET(STDIN_FILENO, &myset);
        select(server_fd + 1, &myset, NULL, NULL, NULL); // This is a wait call

        // recv(server_fd, buffer, 1024, 0); // This stores the servers message into the buffer
        // printf("%s", buffer); // Print out said meassage
        // fflush(stdout); // Clear stdout

        // scanf("%s", buffer); // Take in the client input and store it in the buffer
        // write(server_fd, buffer, 1024); // Give the client input back to the server through the buffer

        if (FD_ISSET(server_fd, &myset)){
          int stuff_read = recv(server_fd, buffer, 1024, 0); // This stores the servers message into the buffer
          if (stuff_read > 0){
            printf("%s", buffer); // Print out said meassage
            fflush(stdout); // Clear stdout
          }
          else{
            fflush(stdout); // Clear stdout
            exit(0);
          }
          
          //printf("Debug test 1");
        }
        if (FD_ISSET(STDIN_FILENO, &myset)){
          scanf("%s", buffer); // Take in the client input and store it in the buffer
          write(server_fd, buffer, 1024); // Give the client input back to the server through the buffer
          //printf("I just wrote something");
        }

        /* Receive response from the server */
        
        // int recvbytes = recv(server_fd, buffer, 1024, 0);

        

        // if (recvbytes == 0) break;
        // else {
        //     buffer[recvbytes] = 0;
        //     printf("%s\n", buffer); //
        //     fflush(stdout);
        //     //write(server_fd, buffer, 1024);
        // }
    }

    close(server_fd);

  return 0;
}
