/**
 * @file server_benchmark.cpp
 * @brief A server-side benchmarking tool for measuring network performance.
 *
 * This program creates a server that listens for incoming client connections
 * and measures the performance of message exchange between the server and
 * the client. The program supports various message sizes and measures end-to-end
 * (E2E) performance.
 */


#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pcap/socket.h>
#include "benchmark.h"


#define PORT                        8011
#define SERVER_QUEUE                50

// Function declarations
void signal_callback_handler(int signum);
static int CreateListeningSocket(void);
static int BenchmarkTest(SOCKET new_socket);
static int BenchmarkMeasurement(SOCKET ipc_socket , int msg_size);
static int SendSocketData(SOCKET ipc_socket);
static int ReceiveSocketData(SOCKET socket_handle, size_t data_len,
							 unsigned char *p_data);


static SOCKET s_ipc_socket = INVALID_SOCKET;
static SOCKET_DATA_T s_socket_data;


/**
 * @brief Main function of the server benchmarking program.
 *
 * The main function initializes and starts the server, then listens for
 * incoming client connections, and performs benchmark tests for each client.
 *
 * @param argc Argument count (should be 1 for this program)
 * @param argv Argument values array (not used)
 * @return STATUS_OK if successful, or exit with an error code otherwise
 */
int main(int argc, char *argv[])
{

    if (argc >1) {
        std::cerr << "Usage: program should be run without any parameter"
        << std::endl;
        exit(EXIT_FAILURE);
    }

    // Register the signal handler for SIGINT to handle program termination
    signal(SIGINT, signal_callback_handler);

  // Create the listening socket for incoming client connections
	if (CreateListeningSocket() != STATUS_OK)
	{
		exit(EXIT_FAILURE);
	}


    SOCKADDR_IN  accept_sin;
    socklen_t    accept_sin_len;
    SOCKET       new_socket;
    accept_sin_len = sizeof accept_sin;

  // Main loop to accept and process client connections
    while (true) {
        new_socket = accept(s_ipc_socket, (struct sockaddr *) &accept_sin,
        	&accept_sin_len);
        if (new_socket == INVALID_SOCKET)
        {
            std::cout << "Server accept FAIL" << GETSOCKETERRNO() << std::endl;
            close(s_ipc_socket);
            std::cout << "socket " << s_ipc_socket << " closed" << std::endl;
            exit(EXIT_FAILURE);
        }
        //std::cout << "Server accepted connection from IPC client: socket=" << new_socket << std::endl;

		// Perform the benchmark test for the connected client
		if (BenchmarkTest(new_socket) != STATUS_OK)
		{
			close(new_socket);
			close(s_ipc_socket);
			std::cout <<
			"BenchmarkMeasurement failed. connection with sockets " <<
			new_socket << "," << s_ipc_socket<< " closed" << std::endl;
			exit(EXIT_FAILURE);		
		}

        else {
            close(new_socket);
        }
    }
  // Close the socket before exiting
    close(s_ipc_socket);
    //std::cout << "socket " << s_ipc_socket << " closed" << std::endl;
    return STATUS_OK;
}



/* --------------------------   external functions implemenation -----------------------*/

/**
 * @brief Signal callback handler function.
 *
 * This function is called when a signal is caught by the program.
 * It will close the IPC socket and terminate the program.
 *
 * @param signum The signal number that was caught.
 */
void signal_callback_handler(int signum) {
    std::cout << "Caught signal " << signum << std::endl;
    // Terminate program
    close(s_ipc_socket);
    std::cout << "socket " << s_ipc_socket << " closed" << std::endl;
    exit(signum);
}

/**
 * @brief Create a listening socket for incoming client connections.
 *
 * This function initializes the server's listening socket, binds it to the
 * given IP and port, and starts listening for incoming client connections.
 *
 * @return STATUS_OK if successful, or STATUS_FAIL otherwise
 */
static int CreateListeningSocket(void)
{
    struct sockaddr_in servaddr;
    int socket_status = SOCKET_ERROR;


  // SOCKET_STARTUP initializes the socket library for the current process.
  // This is typically required for Windows-based systems but not for Unix/Linux systems.
  // It returns zero on success or a non-zero error code on failure.
    socket_status = SOCKET_STARTUP(s_socket_data);  
	
    // socket create and verification
    s_ipc_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s_ipc_socket == INVALID_SOCKET)
    {
        std::cout << " socket creation failed ." << GETSOCKETERRNO() << std::endl;
		return STATUS_FAIL;
    }
	

    //std::cout << "Socket successfully created..." << s_ipc_socket << std::endl;

    memset(&servaddr, 0, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("132.65.164.101");
    servaddr.sin_port = PORT ;
    // Binding newly created socket to given IP and verification
    socket_status = bind(s_ipc_socket, (struct sockaddr*)&servaddr,
    	sizeof servaddr);
    if (socket_status == SOCKET_ERROR)
    {
        std::cout << "socket bind failed..." << GETSOCKETERRNO() << std::endl;
        close(s_ipc_socket);
        std::cout << "socket " << s_ipc_socket << " closed" << std::endl;
		return STATUS_FAIL;
    }

    //std::cout << "Socket successfully binded.." << std::endl;

    // Now server is ready to listen and verification
    if ((listen(s_ipc_socket, SERVER_QUEUE)) == SOCKET_ERROR)
    {
        std::cout << "Listen failed..." <<  GETSOCKETERRNO() << std::endl;
        close(s_ipc_socket);
        std::cout << "socket " << s_ipc_socket << " closed" << std::endl;
		return STATUS_FAIL;
    }

    //std::cout << "Server listening..." << std::endl;
	
	return STATUS_OK;

}


/**
 * @brief Main benchmark test function.
 *
 * This function handles warm-up cycles and the end-to-end benchmarking.
 *
 * @param new_socket The connected client socket.
 * @return STATUS_OK if successful, or STATUS_FAIL otherwise.
 */
static int BenchmarkTest(SOCKET new_socket)
{
	int msg_size = MESSAGE_SIZE_START;
	int status = STATUS_FAIL;

  // Warm-up cycles are performed before the actual benchmarking to allow
  // the system to reach a stable state, reduce initialization overhead,
  // and minimize the impact of transient effects.

	//std::cout << "starting warmup cycles" << std::endl;
	msg_size = MESSAGE_SIZE_START;
	for  (int warmup = 0 ; warmup < BENCHMARK_WARM_UP_CYCLES ; warmup ++)
	{
		while(msg_size <= MESSAGE_SIZE_END)
		{
			status = BenchmarkMeasurement(new_socket ,  msg_size);
			if (status == STATUS_FAIL)
			{
				close(new_socket);
				close(s_ipc_socket);
				std::cout <<
				"BenchmarkMeasurement failed. connection with sockets " <<
				new_socket << "," << s_ipc_socket<< " closed" << std::endl;
				return (STATUS_FAIL);
			}
			msg_size *= MESSAGE_SIZE_MULTIPLIER;
		}
		msg_size = MESSAGE_SIZE_START;
	}
	//std::cout << "finished warmup cycles" << std::endl;
	//std::cout << "starting E2E banchmarking" << std::endl;

  // The main test consists of sending messages of varying sizes between the
  // server and the client, measuring the performance of the IPC mechanism.
	msg_size = MESSAGE_SIZE_START;
	while(msg_size <= MESSAGE_SIZE_END)
	{
		status = BenchmarkMeasurement(new_socket ,  msg_size);
		if (status == STATUS_FAIL)
		{
			close(new_socket);
			close(s_ipc_socket);
			//std::cout << "BenchmarkMeasurement failed. connection with
			// sockets " << new_socket << "," << s_ipc_socket<< " closed"
			// << std::endl;
			return (STATUS_FAIL);
		}
		msg_size *= MESSAGE_SIZE_MULTIPLIER;
	}
	close(new_socket);

	//std::cout << "socket " << new_socket << " closed" << std::endl;
	//std::cout << "finished E2E banchmarking" << std::endl;

	return (STATUS_OK);

}


/**
 * @brief Perform a single benchmark measurement.
 *
 * This function measures the performance of the IPC mechanism
 * for a given message size.
 *
 * @param ipc_socket The connected client socket.
 * @param msg_size The size of the message for this measurement.
 * @return STATUS_OK if successful, or STATUS_FAIL otherwise.
 */
static int BenchmarkMeasurement(SOCKET ipc_socket , int msg_size)
{
    char buff[msg_size];
    memset(buff, 0, sizeof(buff));
    int counter = 0;
    int socket_status = SOCKET_ERROR;
    while (counter < BENCHMARK_NUM_OF_MESSAGES)
    {
        socket_status = ReceiveSocketData(ipc_socket, sizeof(buff), (unsigned char *)buff);
        if (socket_status == SOCKET_ERROR)
        {
            std::cout << "socket erro in E2E banchmark" << std::endl;
            return STATUS_FAIL;
        }
        counter++;
    }

    socket_status = SendSocketData(ipc_socket);
    if (socket_status == SOCKET_ERROR)
    {
        std::cout << "server failed to answer client " << std::endl;
        return STATUS_FAIL;
    }
    //std::cout << "sever answered for message size "<< msg_size << std::endl;
    return STATUS_OK;

}


/**
 * @brief Send data to the client socket.
 *
 * This function sends a fixed size response to the client socket.
 *
 * @param ipc_socket The connected client socket.
 * @return 0 if successful, or SOCKET_ERROR otherwise.
 */
static int SendSocketData(SOCKET ipc_socket)
{
    char buff[SERVER_RESPONSE_SIZE];
    memset(buff, 'x' ,sizeof(buff));
    buff[sizeof(buff) - 1] = 0;

    if (send(ipc_socket, (char *)buff, sizeof(buff), 0) == SOCKET_ERROR)
    {
        return SOCKET_ERROR;
    }

    return 0;
}


/**
 * @brief Receive data from the client socket.
 *
 * This function receives a specified amount of data from the client socket.
 *
 * @param socket_handle The connected client socket.
 * @param data_len The length of the data to receive.
 * @param p_data Pointer to the buffer that will store the received data.
 * @return The result of the recv() call, or SOCKET_ERROR in case of an error.
 */
static int ReceiveSocketData(SOCKET socket_handle, size_t data_len, unsigned char *p_data)
{
    int received = 0;
    int socket_result = 0;
    while (received < data_len) {
        socket_result = recv(socket_handle, p_data + received, data_len - received, 0);
        if (socket_result == -1) {
            std::cerr << "Error: recv failed" << std::endl;
            return socket_result;
        }
        received += socket_result;
    }

    return socket_result;
}






