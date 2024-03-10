#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

// The maximum number of clients that the server can handle.
#define MAX_CLIENT 5
#define BUFFER_SIZE 1024
#define FILE_SIZE 2097152

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        printf("Usage: %s -p PORT -algo ALGO\n", argv[0]);
        return 1;
    }

    int port = 0;
    char *algo = NULL;
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-p") == 0)
        {
            port = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-algo") == 0)

        {
            algo = argv[++i];
        }
    }

    if (port == 0 || algo == NULL)
    {
        printf("Missing arguments. Usage: %s -p PORT -algo ALGO\n", argv[0]);
        return 1;
    }
    // The file to store the received data.
    FILE *file = fopen("received_file.txt", "wb");
    // If the file opening failed, print an error message and return 1.
    if (file == NULL)
    {
        perror("fopen(3)");
        return 1;
    }
    // The variable to store the socket file descriptor.
    int sock = -1;

    // The variable to store the server's address.
    struct sockaddr_in server;
    // The variable to store the client's address.
    struct sockaddr_in client;
    // Stores the client's structure length.
    socklen_t client_len = sizeof(client);
    // Create a message to send to the client.
    char *message = "The file has been received!\n";

    int messageLen = strlen(message) + 1;

    struct timeval timeStart, timeEnd;
    // The variable to store the socket option for reusing the server's address.
    int opt = 1;
    // Reset the server and client structures to zeros.
    memset(&server, 0, sizeof(server));
    memset(&client, 0, sizeof(client));
    // Try to create a TCP socket (IPv4, stream-based, default protocol).
    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == -1)
    {
        perror("socket(2)");
        return 1;
    }
    
    // Set the socket option to reuse the server's address.
    // This is useful to avoid the "Address already in use" error message when restarting the server.
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt(2)");
        close(sock);
        return 1;
    }
    // Set the congestion control algorithm
    if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, algo, strlen(algo)) != 0)
    {
        perror("setsockopt");
        close(sock);
        return 1;
    }
    

    // Set the server's address to "0.0.0.0" (all IP addresses on the local machine).
    server.sin_addr.s_addr = INADDR_ANY;

    // Set the server's address family to AF_INET (IPv4).
    server.sin_family = AF_INET;

    // Set the server's port to the specified port. Note that the port must be in network byte order.
    server.sin_port = htons(port);

    // Try to bind the socket to the server's address and port.
    if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("bind(2)");
        close(sock);
        return 1;
    }

    // Try to listen for incoming connections.
    if (listen(sock, MAX_CLIENT) < 0)
    {
        perror("listen(2)");
        close(sock);
        return 1;
    }
    fprintf(stdout, "Starting the Receiver...\n");
    fprintf(stdout, "Listening for incoming connections on port %d...\n", port);
    // The structure to store the time and bandwidth of the received files.
    typedef struct {
        double time;
        double bandwidth;
    } TimeBandwidth;
    
    int arrSize = 8;
    // The variable to store the number of run times.
    int TIME = 0;
    // The array to store the times of the received files.
    TimeBandwidth *times_bandwith = (TimeBandwidth *)malloc(arrSize * sizeof(TimeBandwidth));
    // Check if the allocation failed.
    if (times_bandwith == NULL)
    {
        perror("malloc");
        close(sock);
        return 1;
    }
    
    // Accept a connection from the sender
    int sender_sock = accept(sock, (struct sockaddr *)&client, &client_len);
    fprintf(stdout, "Waiting for a sender to connect...\n");

    // If the message receiving failed, print an error message and return 1.
    if (sender_sock < 0)
    {
        perror("accept(2)");
        close(sock);
        return 1;
    }
    // Print a message to the standard output to indicate that a new client has connected.
    fprintf(stdout, "Sender %s:%d connected, begging to receive file\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
    // The variable to store the time it took to receive the file.
    double cpu_time_used;
    double total_time = 0;
    double total_bandwidth = 0;
    int num_files = 0;
    
    
    // The receiver's main loop.
    while (1)
    {
        
        fclose(file);
        file = fopen("received_file.txt", "wb");

        if (file == NULL)
        {
            perror("fopen(3)");
            return 1;
        }
        // Start counting time
        gettimeofday(&timeStart, NULL);
        int total_bytes_received = 0;
        // The buffer to store the received data.
        char buffer[FILE_SIZE] = {0};
        // The buffer to store the received message.
        char bufferM[BUFFER_SIZE] = {0};
        int bytes_received = 0;
        int bytes_sent = 0;
        // The inner loop to receive the file.
        while (total_bytes_received < FILE_SIZE)
        {
            // Try to receive a message from the client using the socket and store it in the buffer.
            bytes_received = recv(sender_sock, buffer, sizeof(buffer), 0);
            printf("Packet size: %d\n", bytes_received);

            // If the message receiving failed, print an error message and return 1.
            if (bytes_received < 0)
            {
                perror("rec(2)");
                close(sender_sock);
                close(sock);
                return 1;
            }

            // If the amount of received bytes is 0, the client has disconnected.
            // Close the client's socket and continue to the next iteration.
            else if (bytes_received == 0)
            {
                fprintf(stdout, "Sender %s:%d disconnected\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
                break;
            }

            total_bytes_received += bytes_received;
            // Write the received data to the file.
            fwrite(buffer, 1, bytes_received, file);
            
                // If the times array is full, reallocate it to double the size.
                if (TIME >= arrSize)
                {
                    arrSize *= 2;
                    ;
                    times_bandwith = (TimeBandwidth *)realloc(times_bandwith, arrSize * sizeof(TimeBandwidth));
                }

            // If we've received the full file
            if (total_bytes_received >= FILE_SIZE)
            {
                fprintf(stdout, "File transfer complete!\n");
                // Stop counting time
                gettimeofday(&timeEnd, NULL);
                // Calculate the time it took to receive the file in milliseconds.
                cpu_time_used = (timeEnd.tv_sec + -timeStart.tv_sec) * 1000 + (((double)timeEnd.tv_usec - timeStart.tv_usec)) / 1000;
                times_bandwith[TIME].time = cpu_time_used;
                double time_seconds = cpu_time_used / 1000; // Convert to seconds
                double total_data_MB = total_bytes_received / (1024 * 1024); // Convert to MB
                // Calculate the bandwidth in MB/s
                double bandwidth = (total_data_MB / time_seconds);
                times_bandwith[TIME].bandwidth = bandwidth;
                
                total_time += cpu_time_used;
                total_bandwidth += bandwidth;
                num_files++;

                TIME++;
                printf("Time: %d\n", TIME);
                printf("Total bytes received: %d\n", total_bytes_received);
                printf("Total data received: %0.3f MB\n", total_data_MB);
                // Send a message to the client to indicate that the file has been received.
                bytes_sent = send(sender_sock, message, messageLen, 0);
                // If the message sending failed, print an error message and return 1.
                if (bytes_sent < 0)
                {
                    perror("send(2)");
                    close(sender_sock);
                    close(sock);
                    return 1;
                }

                if (bytes_sent == 0)
                {
                    fprintf(stdout, "Sender %s:%d disconnected\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
                    break;
                }
                break; // Break out of the inner loop
            }
        }

        
        // Wait for sender response
        fprintf(stdout, "Waiting for the sender to respond...\n");
        if (recv(sender_sock, bufferM, sizeof(bufferM), 0) < 0)
        {
            perror("recv(2)");
            close(sender_sock);
            close(sock);
            return 1;
        }
        // If the sender has closed the connection, or sent an exit message break out of the loop
        if (strcmp(bufferM, "Exit") == 0 || total_bytes_received == 0)
        {
            printf("The Sender has closed the TCP connection.\n");
            break;
        }

        
        else
        {
            printf("The sender is still connected\n");
        }

        fprintf(stdout, "Sent %d bytes to the sender %s:%d!\n", bytes_sent, inet_ntoa(client.sin_addr), ntohs(client.sin_port));
    }
    // Close the client's socket.
    close(sender_sock);
    fclose(file);
    fprintf(stdout, "Sender %s:%d disconnected\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
    double avarage = 0;
    printf("\n----------------------------------\n");
    printf("* %s Statistics: *\n", algo);
    // Print the times and bandwith of the received files.
    for (int i = 0; i < TIME; i++)
    {
        avarage += times_bandwith[i].time;
        printf("Run %d Data: time = %0.3lf ms; Speed = %0.3f MB/s \n", (i + 1), times_bandwith[i].time, times_bandwith[i].bandwidth);
    }
    avarage = avarage / TIME;
    printf("The total time: %0.3lf ms\n", total_time);
    printf("The avarage time: %0.3lf ms\n", avarage);
    printf("The total bandwidth: %0.3lf MB/s\n", total_bandwidth);
    printf("The average bandwidth: %0.3lf MB/s\n", (total_bandwidth / num_files));
    printf("----------------------------------\n");

    // Close the server's socket.
    close(sock);
    fprintf(stdout, "Receiver finished!\n");
    // Free the memory allocated for the times array.
    free(times_bandwith);

    return 0;
}
