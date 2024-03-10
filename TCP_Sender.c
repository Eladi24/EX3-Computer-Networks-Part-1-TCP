#include <stdio.h> // Standard input/output library
#include <arpa/inet.h> // For the in_addr structure and the inet_pton function
#include <sys/socket.h> // For the socket function
#include <unistd.h> // For the close function
#include <string.h> // For the memset function
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <time.h>

/*
 * @brief The buffer size to store the received message.
 * @note The default buffer size is 1024.
*/
#define BUFFER_SIZE 1024

#define RECEIVER_IP_ADDRESS "127.0.0.1"
#define SERVER_PORT 9999
#define FILE_SIZE 2097152
/*
* @brief
A random data generator function based on srand() and rand().
* @param size
The size of the data to generate (up to 2^32 bytes).
* @return
A pointer to the buffer.
*/

char *util_generate_random_data(unsigned int size) {
    char *buffer = NULL;

     if (size <= 0)
     {
        return NULL;
     }

     buffer = (char *)calloc(size, sizeof(char));

     if (buffer == NULL) {
        return NULL;
     }

     // Randomize the seed of the random number generator.
     srand(time(NULL));

     for (unsigned int i = 0; i < size; i++)
     {
        *(buffer + i) = ((unsigned int)rand() % 256);
     }

     return buffer;
}

/*
 * @brief TCP Sender main function.
 * @param None
 * @return 0 if the client runs successfully, 1 otherwise.
*/

int main(int argc, char *argv[]) {
    if (argc != 7)
    {
        printf("Usage: %s -ip IP -p PORT -algo ALGO\n", argv[0]);
        return 1;
    }

    char *ip = NULL;
    int port = 0;
    char *algo = NULL;

    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-ip") == 0)
        {
            ip = argv[++i];
        } else if (strcmp(argv[i], "-p") == 0)
        {
            port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-algo") == 0)
        {
            algo = argv[++i];
        }
    }

    if (ip == NULL || port == 0 || algo == NULL) {
        printf("Missing arguments. Usage: %s -ip IP -p PORT -algo ALGO\n", argv[0]);
        return 1;
    }
    
    // The variable to store the socket file descriptor.
    int sock = -1;

    // The variable to store the server's address.
    struct sockaddr_in receiver;
    // Create a file to send to the server.
    //char *file = util_generate_random_data(FILE_SIZE);
    char *data = util_generate_random_data(FILE_SIZE);

    // If the file creation failed, print an error message and return 1.
    if (data == NULL)
    {
        perror("util_generate_random_data");
        return 1;
    }

    
    FILE *fptr = fopen("2MBFile.txt", "wb");
    if (fptr == NULL)
    {
        perror("fopen");
        return 1;
    }
    // Write the data to the file.
    size_t bytes_written = fwrite(data, sizeof(char), FILE_SIZE, fptr);
    if (bytes_written != FILE_SIZE)
    {
        perror("fwrite");
        return 1;
    }
    fclose(fptr);

    fptr = fopen("2MBFile.txt", "r"); // Open the file in read mode.
    
    // If the file opening failed, print an error message and return 1.
    if (fptr == NULL)
    {
        perror("fopen");
        return 1;
    }
    fseek(fptr, 0, SEEK_END); // Move the file pointer to the end of the file.
    long fileSize = ftell(fptr); // Get the current position of the file pointer.
    fprintf(stdout, "The file size is: %ld bytes\n", fileSize);
    fseek(fptr, 0, SEEK_SET); // Move the file pointer to the beginning of the file.
    
    // Create a buffer to store the received message
    char bufferSend[FILE_SIZE] = {0};
    // Reset the server structure to zeros.
    memset(&receiver, 0 ,sizeof(receiver));
    // Try to create a TCP socket (IPv4, stream-based, default protocol).
    sock = socket(AF_INET, SOCK_STREAM, 0);

    // If the socket creation failed, print an error message and return 1.
    if (sock == -1)
    {
        perror("socket(2)");
        return 1;
    }
    
    if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, algo, strlen(algo)) != 0)
    {
        perror("setsockopt");
        return 1;
    }
    

    // Convert the server's address from text to binary form and store it in the server structure.
    // This should not fail if the address is valid (e.g. "127.0.0.1").
    if(inet_pton(AF_INET, ip, &receiver.sin_addr) <= 0) {
        perror("inet_pton(3)");
        close(sock);
        return 1;
    }

    // Set the server's address family to AF_INET (IPv4).
    receiver.sin_family = AF_INET;
    // Set the server's port to the defined port. Note that the port must be in network byte order,
    // so we first convert it to network byte order using the htons function. 
    receiver.sin_port = htons(port);

    fprintf(stdout, "Connecting to %s:%d...\n", ip, port);
    

    // Try to connect to the server using the socket and the server structure.
    int connection = connect(sock, (struct sockaddr*)&receiver, sizeof(receiver));
    if (connection < 0)
    {
        perror("connect(2)");
        close(sock);
        return 1;
    }
    
    fprintf(stdout, "Successfully connected to the Receiver!\n"
                    "Sending file to the Receiver: %s\n", "2MBFile.txt");
    char choice = 'Y';
    
    while(choice == 'Y' || choice == 'y')
    {
        int bytes_sent = 0;
        int bytes_received = 0;
        int total_bytes_sent = 0;
        
        fseek(fptr, 0, SEEK_SET);
        
        while ((connection = fread(bufferSend, sizeof(char), FILE_SIZE, fptr)) > 0)
        {
            bytes_sent = send(sock, bufferSend, connection, 0);
            if (bytes_sent <= 0)
            {
                perror("send(2)");
                close(sock);
                return 1;
            } 
            
            total_bytes_sent += bytes_sent;
            fprintf(stdout, "Sent %d bytes to the receiver!\n", bytes_sent);
        }
    
    fprintf(stdout, "The file was sent to the Receiver! Total bytes: %d\n"
    "Waiting for the receiver to respond...\n", total_bytes_sent);
    char buffer[BUFFER_SIZE] = {0};
    // Try to receive a message from the server using the socket and store it in the buffer.
    bytes_received = recv(sock, buffer, sizeof(buffer), 0);
    // If the message receiving failed, print an error message and return 1.
    // If no data was received, print an error message and return 1. Only occurs if the connection was closed.
    if (bytes_received <= 0)
    {
        perror("recv(2)");
        close(sock);
        return 1;
    }
    
    
    // Ensure that the buffer is null-terminated, no matter what message was received.
    // This is important to avoid SEGFAULTs when printing the buffer.
    if (buffer[BUFFER_SIZE - 1] != '\0')
    {
        buffer[BUFFER_SIZE - 1] = '\0';
    }
    
    fprintf(stdout, "Got %d bytes from the Receiver, which says: %s\n", bytes_received, buffer);
        
        printf("Do you want to send the file again?[Y/N]\n");
        scanf(" %c", &choice);
        if (choice == 'n' || choice == 'N')
        {
            // Sends exit message to the receiver.
            send(sock, "Exit", strlen("Exit"), 0);
            break;
        } else if (choice != 'Y' && choice != 'y' && choice != 'n' && choice != 'N')
        {
            printf("Press only : Y - Send again || N - Close connection \n");
            scanf(" %c", &choice);
        }
        // Sends keep alive message to the receiver.
        send(sock, "KeepAlive", strlen("KeepAlive"), 0);
    } 
    
    // Close the socket with the server.
    close(sock);
    fclose(fptr);
    fprintf(stdout, "Connection closed!\n");
    free(data);
    
    return 0;
    
}