/*** socket/demo2/server.c ***/

/*******************************************************************************
 * Name        : server.c
 * Author      : Yash Yagnik
 ******************************************************************************/

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>

// 4/30

// Problems:
// When I control c out of one of the clients then it just infinite loops and print the answer of the question

// Final 5/2/24 5:38pm


int main(int argc, char* argv[]){
    int    server_fd;
    int    client_fd;
    struct sockaddr_in server_addr;
    struct sockaddr_in in_addr;
    socklen_t addr_size = sizeof(in_addr);

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
                printf("Usage: %s [-f question_file] [-i IP_address] [-p port_number] [-h]\n\n\t-f question_file Default to '%s' ;\n\t-i IP_address Default to '%d.%d.%d.%d';\n\t-p port_number Default to 25555;\n\t-h Display this help info.\n", argv[0], "questions.txt" , 127 , 0, 0, 1);
                return EXIT_SUCCESS;
                break;
            case '?':
                fprintf(stderr, "Error: Unknown option '%s' received\n", argv[1]); // This is if the user enters an invalid flag
                return EXIT_FAILURE;
                break;
        }

    }

        

    /* STEP 1
        Create and set up a socket
    */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == -1) {
        perror("Error: Socket creation failed");
        exit(EXIT_FAILURE);
    }
    

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(port_number);
    server_addr.sin_addr.s_addr = inet_addr(IP_address);

    /* STEP 2
        Bind the file descriptor with address structure
        so that clients can find the address
    */
    // bind(server_fd,
    //         (struct sockaddr *) &server_addr,
    //         sizeof(server_addr));

    if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
        perror("Error: Socket bind failed");
        exit(EXIT_FAILURE);
    }

    /* STEP 3
        Listen to at most 5 incoming connections
    */

    #define MAX_CONN 3
    if (listen(server_fd, 3) == 0){
        printf("Welcome to 392 Trivia!\n");
        //return 0;
    }
    else{
        perror("Error: Listen function failed");
        return 0;
    }

    struct Player {
        int fd;
        int score;
        char name[128];
    };

    struct Entry {
        char prompt[1024];
        char options[3][50];
        int answer_idx;
    };

    

    struct Entry questions[50];
    struct Player players[3];

    int read_questions(struct Entry* arr, char* filename){
        FILE *open_question_file;
        //printf("I am in the read questions function");
        if ((open_question_file = fopen(filename, "r")) == NULL){
            perror("Error: Unable to open file (fopen function failed)");
            exit(1);
        }
        char* line = NULL;
        size_t len = 0;
        ssize_t read;
        int entry_count = 0;

        while ((read = getline(&line, &len, open_question_file)) != -1 && entry_count < 51) {
            //char* options[];
            // Read prompt
            line[strcspn(line, "\n")] = 0; // Remove newline character
            //printf("Question %d: %s\n", entry_count + 1, line);
            strcpy(arr[entry_count].prompt, line);
            

            // Read options
            
            if ((read = getline(&line, &len, open_question_file)) != -1) {
                line[strcspn(line, "\n")] = 0; // Remove newline character
                //printf("Read options %n: %s\n", entry_count + 1, line);
                char* token = strtok(line, " ");
                int i = 0;
                while (token != NULL){
                    strcpy(arr[entry_count].options[i], token);
                    //printf("%d: %s\n", i+1, arr[entry_count].options[i]);
                    //arr[entry_count].options[i] = token; // Come back to this prob; I guess im assigning an array to an arry and that is why it is getting mad
                    i++;
                    token = strtok(NULL, " ");
                }
            } 
            else {
                // Handle unexpected end of file or format error
                perror("Error: Question options cannot be read");
                exit(1);
            }

            // Read answer index
            if ((read = getline(&line, &len, open_question_file)) != -1) {
                // Skip over blank lines and trim newline characters
                line[strcspn(line, "\n")] = 0; // Remove newline character
                //printf("Answer line: %s\n\n", line); // Debug print answer line
                for (int i = 0; i < 3; i++) {
                    if (strcmp(line, arr[entry_count].options[i]) == 0) {
                        arr[entry_count].answer_idx = i + 1;
                    }
                }
            } 
            else {
                perror("Error: Question answers cannot be read");
                exit(1);
            }

            while ((read = getline(&line, &len, open_question_file)) != -1 && strlen(line) == 0) {
            // Skip blank lines inbetween the answer and the next question
            }
        
            entry_count++;
        }
            return entry_count + 1;
            fclose(open_question_file);
            free(line);
    }

    /* STEP 4
        Accept connections from clients
        to enable communication
    */

    //int MAX_CONN = 3;
    fd_set myset;
    FD_SET(server_fd, &myset);

    int max_fd = server_fd;
    int n_conn = 0;
    int cfds[MAX_CONN];

    for (int i = 0; i < MAX_CONN; i++){
        cfds[i] = -1;
    }

    //char* user_name;
    char* receipt   = "Read\n";
    int   recvbytes = 0;
    char  buffer[1024];
    int ready = 0;
    //int game_running = 0;

    int num_of_questions = read_questions(questions, question_file); // Read the questions before we go into the while loop to start the game

    while (1){
        // Re-intalizing the file descriptor
        FD_SET(server_fd, &myset);
        max_fd = server_fd;
        for (int i = 0; i < MAX_CONN; i++){
            if (cfds[i] != -1){
                FD_SET(cfds[i], &myset);
                if(cfds[i] > max_fd){
                    max_fd = cfds[i];
                }
            }
        }

        // Montitoring fd
        select(max_fd + 1, &myset, NULL, NULL, NULL);

        // If there is a new connection
        if (FD_ISSET(server_fd, &myset)){
            
            client_fd  =   accept(server_fd, (struct sockaddr*)&in_addr, &addr_size);

            if (client_fd == -1) {
                perror("Error: Accept function failed");
                continue; // or handle the error accordingly
            }

            if (n_conn < MAX_CONN){
                n_conn++;
                printf("New connection Detected!\n");
                write(client_fd, "Please type your name: ", 24);
                //int bytes_read = read(client_fd, buffer, sizeof(buffer)); // Put a error check here if the user does not enter in a name

                // printf("Hi %s!\n", buffer); // here is yash is printing
                // ready++;
                
                for (int i = 0; i < MAX_CONN; i++){
                    if (cfds[i] == -1){
                        cfds[i] = client_fd;
                        break;
                    }
                }
            }
            else{
                printf("Max connections reached!\n"); // You can start the game from here
                close(client_fd); // This seems to not work
            }  
        }

        
        for (int i = 0; i < MAX_CONN; i++){
            if (cfds[i] !=1 && FD_ISSET(cfds[i], &myset)){
                recvbytes = read(cfds[i], buffer, 1024);
                if (recvbytes > 0){

                        //game_running = 1;
                        buffer[recvbytes] = 0;
                        players[i].fd = cfds[i];
                        players[i].score = 0;
                        strcpy(players[i].name, buffer);
                        printf("Hi %s!\n", buffer); // here is yash is printing
                        ready++;
                    
                    
                    
                        if (ready == MAX_CONN){
                            //fflush(stdout); 
                            printf("The game begins now!\n");
                            //printf("I am going to the read questions function");
                            
                            // Send questions to clients one by one
                            for (int q = 0; q < (num_of_questions - 1); q++) {  // Loop through all the questions
                                FD_SET(server_fd, &myset);
                                max_fd = server_fd;
                                for (int i = 0; i < MAX_CONN; i++){
                                    if (cfds[i] != -1){
                                        FD_SET(cfds[i], &myset);
                                        if(cfds[i] > max_fd){
                                            max_fd = cfds[i];
                                        }
                                    }
                                }
                                // Send the question to each connected client
                                printf("Question %d: %s\n%d: %s\n%d: %s\n%d: %s\n", q+1, questions[q].prompt, 1, questions[q].options[0], 2, questions[q].options[1], 3, questions[q].options[2]);

                                // Reset all the file descriptors to be able to read from any client
                                for (int i = 0; i < MAX_CONN; i++) {
                                    if (cfds[i] != -1) {
                                        // Write the question prompt to the client
                                        char qmess[1024];
                                        snprintf(qmess, sizeof(qmess), "Question %d: %s\nPress %d: %s\nPress %d: %s\nPress %d: %s\n\n", q+1, questions[q].prompt, 1, questions[q].options[0], 2, questions[q].options[1], 3, questions[q].options[2]);
                                        write(cfds[i], qmess, sizeof(qmess)); // This call gets made cuz i see this output 
                                    }
                                }

                                select(max_fd+1, &myset, NULL, NULL, NULL); // This is a wait call
                                for (int i = 0; i < MAX_CONN; i++) {
                                    //printf("yo give me response\n"); // this does print out
                                    char buffer[1024];
                                    //FD_ZERO(&myset);
                                    //FD_SET(client_fd, &myset);
                                    if (FD_ISSET(cfds[i], &myset)){ // checking if the client is ready to respond
                                            //printf("chill im responding"); // this does not print out
                                            int bytes_read = read(cfds[i], buffer, 1024);
                                            
                                            if (bytes_read > 0){
                                                char ans[1024];
                                                snprintf(ans, sizeof(ans), "Answer: %s\n\n", questions[q].options[questions[q].answer_idx - 1]);
                                                for (int i = 0; i < MAX_CONN; i++) {
                                                    write(cfds[i], ans, sizeof(ans)); // Sending out the answer to the clients after one responds
                                                }
                                                //buffer[strcspn(buffer, "\n")] = 0; // Remove newline character
                                                if (atoi(buffer) == questions[q].answer_idx){
                                                    for (int temp = 0; temp < 3; temp++){
                                                        if (players[temp].fd == cfds[i]){
                                                            players[temp].score = players[temp].score + 1; // Add the points if the answer is right
                                                            //printf("%s's score is %d\n", players[temp].name, players[temp].score);
                                                        }
                                                    }
                                                    //printf("Correct Answer broski\n");
                                                }
                                                else{
                                                    for (int temp = 0; temp < 3; temp++){
                                                        if (players[temp].fd == cfds[i]){
                                                            players[temp].score = players[temp].score - 1; // Remove a point if the answer is wrong
                                                            //printf("%s's score is %d\n", players[temp].name, players[temp].score);
                                                        }
                                                    }
                                                }
                                                //fflush(stdout); // Clear stdout

                                            }
                                            
                                            else{
                                                n_conn--;
                                                close(cfds[i]);
                                                cfds[i] = -1;
                                                printf("Conneciton lost\n");
                                                return 0;
                                            } // This is good but the problem is that it presents the questions one at a time not all at once I think that has to do with read and write being in the same loop
                                            //printf("User Answer: %s\n", buffer);// Process the client's answer
                                            
                                    }
                                }
                            }
                            int winner_index1 = 0;
                            char* winners[3];
                            for (int dawg = 0; dawg < 2; dawg++){ // This for loop calculates the highest score
                                if (players[winner_index1].score > players[dawg+1].score){
                                    winner_index1 = dawg;
                                }
                                else{
                                    winner_index1 = (dawg + 1);
                                }
                            }

                            winners[0] = players[winner_index1].name;
                            int another_winner = 1;
                            for (int dawg = 0; dawg < 3; dawg++){
                                if (dawg != winner_index1){
                                    if (players[dawg].score == players[winner_index1].score){
                                        winners[another_winner] = players[dawg].name;
                                        another_winner++;
                                    }
                                }
                            }
                            printf("Congrats, ");
                            for (int i = 0; i < another_winner; i++){
                                if (i == (another_winner - 1)){
                                    printf("%s", winners[i]);
                                }
                                else{
                                    printf("%s and ", winners[i]);
                                }
                                
                            }
                            printf("!\n");
                            //printf("%s with %d", winners[0], players[winner_index1].score);
                            // if (winner_index2 == -1){
                            //     printf("Congrats, %s!\n", players[winner_index1].name);
                            // }
                            // else if (winner_index3){
                            //     printf("Congrats, %s and %s!\n", players[winner_index1].name, players[winner_index2].name);
                            // }
                            // WHAT IF 3 PLAYERS GET A TIE
                            
                            fflush(stdout); // Clear stdout
                            exit(0);
                            //break; // This start taking the numbers as names now and saying hi to them
                        }
                    //write(cfds[i], receipt, strlen(receipt)); // This is write call that prints read in the client
                }
                else if (recvbytes == 0){
                    n_conn--;
                    close(cfds[i]);
                    cfds[i] = -1;
                    printf("Conneciton lost\n");
                    return 0;
                }
            }
            if (recvbytes == -1){
                perror("Error: Reading from client failed");
                // handle the error (e.g., close connection, continue the loop, etc.)
            }
        }
    }
    close(server_fd);
    return 0;

    while(1) {
        recvbytes = recv(client_fd, buffer, 1024, 0);
        //int bytes_read = read(client_fd, buffer, sizeof(buffer)); // Put a error check here if the user does not enter in a name

        // printf("Hi %s!\n", buffer); // here is yash is printing
        // ready++;
        if (recvbytes == 0) break;
        else {
            buffer[recvbytes] = 0;
            printf("[Client]: %s", buffer);
            send(client_fd, receipt, strlen(receipt), 0);
        }
    }

    printf("Ending....\n");
    close(client_fd);
    close(server_fd);

    return 0;
}
