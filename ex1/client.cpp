/**
 * @file benchmark.cpp
 * @brief This file contains the main function and utility functions for
 *        performing an end-to-end communication benchmark test.
 */

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

using namespace std;

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string>
#include <sstream>
#include <pcap/socket.h>
#include "benchmark.h"

/**
 * @brief Structure to hold data for each benchmark measurement.
 */
typedef struct IPC_CONNECTION_MEASURE_T
{
    int size;              ///< Message size in bytes.
    unsigned char* p_msg;  ///< Pointer to the message content.
    double throughput;     ///< Throughput in bytes per microsecond.
} IPC_CONNECTION_MEASURE_T;

// Function declarations

static int CreateConnectSocket(SOCKET *p_ipc_socket , const char* ip ,
							   int port);
static int BenchmarkMeasurement(SOCKET ipc_socket ,
								IPC_CONNECTION_MEASURE_T * measurement_data);
static int E2E_communication(SOCKET ipc_socket ,
							 IPC_CONNECTION_MEASURE_T * measurement_data);
static int GenerateBanchmarkData(IPC_CONNECTION_MEASURE_T *p_measurement_data ,
								 int message_size);
static void FreeBanchmarkData(IPC_CONNECTION_MEASURE_T **p_measurement_data);


static int SendSocketData(SOCKET ipc_socket, unsigned char *data,
						  size_t data_len);
static int ReceiveSocketData(SOCKET socket_handle);
static void WarmUP(SOCKET ipc_socket);
static void parseUrl(const std::string& url, std::string& host,
					 std::string& port);



/**
 * @brief Main function to perform benchmark tests.
 *
 * @param argc The number of arguments.
 * @param argv The array of argument values.
 * @return int EXIT_SUCCESS if the program runs successfully, EXIT_FAILURE otherwise.
 */
int main(int argc, char *argv[])
{
    SOCKET ipc_socket = INVALID_SOCKET;
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <ip>:<port>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string server_address = argv[1];
    std::string ip_str, port_str;
    parseUrl(server_address, ip_str, port_str);

    const char *ip =  ip_str.c_str();
    int port = std::stoi(port_str);


    if (CreateConnectSocket(&ipc_socket , ip , port) != STATUS_OK)
    {
        cout << "socket creation and connect to the server failed" << std::endl;
        return EXIT_FAILURE;
    }

    // warm-up cycles

    //std::cout << "starting warmup cycles" << std::endl;
    WarmUP(ipc_socket);

    //std::cout << "finished warmup cycles" << std::endl;
    //std::cout << "starting E2E banchmarking" << std::endl;

    for (int size = MESSAGE_SIZE_START; size <= MESSAGE_SIZE_END; size *= MESSAGE_SIZE_MULTIPLIER) {
        auto * p_measurement_data = (IPC_CONNECTION_MEASURE_T *) malloc(sizeof (IPC_CONNECTION_MEASURE_T));
        GenerateBanchmarkData(p_measurement_data , size);
        if (BenchmarkMeasurement(ipc_socket ,  p_measurement_data) != STATUS_OK)
        {
            cout << " failed to run the banchmark tests for message size: " << size  << endl;
        }
        FreeBanchmarkData(&p_measurement_data);
    }

    // close the socket
    close(ipc_socket);

    return 0;
}

/**
 * @brief Parses a URL and extracts the host and port components.
 *
 * @param url The URL to parse.
 * @param host The extracted host component.
 * @param port The extracted port component.
 */
void parseUrl(const std::string& url, std::string& host, std::string& port) {
    std::stringstream ss(url);
    std::getline(ss, host, ':');   // extract host component
    std::getline(ss, port);  // extract port component
}


/**
 * @brief Calculates the throughput given the message size and duration.
 *
 * @param msg_size The size of the message in bytes.
 * @param duration The duration of the communication in microseconds.
 * @return double The calculated throughput in bytes per microsecond.
 */
static double calculate_throughput(int msg_size , unsigned long duration)
{
//    std::cout << "duration" << duration << " num total bytes"<< msg_size * BENCHMARK_NUM_OF_MESSAGES<< std::endl;
    return (double)(msg_size * BENCHMARK_NUM_OF_MESSAGES )/ (double)duration;
}


/**
 * @brief Performs warm-up cycles for the benchmark test.
 *
 * @param ipc_socket The IPC socket to use for communication.
 */
static void WarmUP(SOCKET ipc_socket){
    for (int i = 0; i < BENCHMARK_WARM_UP_CYCLES; ++i) {
        //std::cout << "warmup cycle: " << i << std::endl;
        for (int size = MESSAGE_SIZE_START; size <= MESSAGE_SIZE_END; size *= MESSAGE_SIZE_MULTIPLIER) {
            auto * p_measurement_data = (IPC_CONNECTION_MEASURE_T *) malloc(sizeof (IPC_CONNECTION_MEASURE_T));
            GenerateBanchmarkData(p_measurement_data , size);
            if (E2E_communication(ipc_socket ,  p_measurement_data) != STATUS_OK)
            {
                std::cout << " failed to run the banchmark tests for message size: " << size  << endl;
            }
            FreeBanchmarkData(&p_measurement_data);
            }
        }
}


/**
 * @brief Creates and connects a socket to the specified IP address and port.
 *
 * @param[out] p_ipc_socket A pointer to the created IPC socket.
 * @param[in] ip The IP address to connect to.
 * @param[in] port The port number to connect to.
 * @return int STATUS_OK if successful, STATUS_FAIL otherwise.
 */
static int CreateConnectSocket(SOCKET *p_ipc_socket , const char * ip, int port )
{
    int status = STATUS_FAIL;
    int socket_status = SOCKET_ERROR;
    struct sockaddr_in servaddr;
    struct addrinfo hints;
    static SOCKET_DATA_T socket_data;

    do
    {
        socket_status = SOCKET_STARTUP(socket_data);
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM; /* TCP stream sockets */
        *p_ipc_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (*p_ipc_socket == INVALID_SOCKET)
        {
            std::cout << "socket creation failed " << GETSOCKETERRNO() << std::endl;
            break;
        }
        else {
            //std::cout << "socket created succssesfuly" << std::endl;
        }

        memset(&servaddr, 0, sizeof(servaddr));

        // assign IP, PORT
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = inet_addr(ip);
        servaddr.sin_port = port;


        // connect the client socket to server socket
        socket_status = connect(*p_ipc_socket, (struct sockaddr*)&servaddr, sizeof(servaddr));
        if (socket_status == SOCKET_ERROR)
        {
            std::cerr << "connection with the server failed  " << GETSOCKETERRNO() << std::endl;
            break;
        }

        status = STATUS_OK;
    } while (false);

    return status;


}

/**
 * @brief Performs a benchmark measurement for a specific message size.
 *
 * @param ipc_socket The IPC socket to use for communication.
 * @param measurement_data A pointer to the measurement data structure.
 * @return int STATUS_OK if successful, STATUS_FAIL otherwise.
 */
static int BenchmarkMeasurement(SOCKET ipc_socket , IPC_CONNECTION_MEASURE_T * measurement_data)
{
    //std::cout << "start BenchmarkMeasurement for size: " << measurement_data->size << std::endl;
    clock_t start;
    unsigned long duration;
    start = clock();
    int status = E2E_communication(ipc_socket , measurement_data);
    if (status == STATUS_FAIL)
    {
        return STATUS_FAIL;
    }
    duration = clock() - start ;
    //std::cout << "Operation took "<< duration << " micro seconds" << std::endl;
    measurement_data->throughput = calculate_throughput(measurement_data->size , duration);
    std::cout << measurement_data->size << "\t" << measurement_data->throughput  << "\t" << "Bytes/Microsecond" << std::endl;
    return STATUS_OK;
}

/**
 * @brief Performs end-to-end communication for a specific message size.
 *
 * @param ipc_socket The IPC socket to use for communication.
 * @param measurement_data A pointer to the measurement data structure.
 * @return int STATUS_OK if successful, STATUS_FAIL otherwise.
 */
static int E2E_communication(SOCKET ipc_socket , IPC_CONNECTION_MEASURE_T * measurement_data){
    int counter ;
    int status = STATUS_FAIL;
    for (counter = 0; counter < BENCHMARK_NUM_OF_MESSAGES; counter ++)
    {
        status = SendSocketData(ipc_socket , measurement_data->p_msg, measurement_data->size);
        if (status == STATUS_FAIL)
        {
            return STATUS_FAIL;
        }
    }
    int num_bytes = ReceiveSocketData(ipc_socket);
    if (num_bytes == SERVER_RESPONSE_SIZE)
    {
        return STATUS_OK;

    }
    return STATUS_FAIL;
}

/**
 * @brief Generates benchmark data for a specific message size.
 *
 * @param[out] p_measurement_data A pointer to the measurement data structure.
 * @param[in] message_size The size of the message in bytes.
 * @return int STATUS_OK if successful, STATUS_FAIL otherwise.
 */
static int GenerateBanchmarkData(IPC_CONNECTION_MEASURE_T *p_measurement_data , int message_size)
{
    p_measurement_data->size = message_size ;
    p_measurement_data->p_msg = (unsigned char *)malloc(p_measurement_data->size);
    if (p_measurement_data->p_msg== nullptr)
        {
            return STATUS_FAIL;
        }
    memset((void *)(p_measurement_data->p_msg), 65, p_measurement_data->size);
    p_measurement_data->p_msg[message_size- 1] = 0;
    return STATUS_OK;
}

/**
 * @brief Frees the memory allocated for the benchmark data structure.
 *
 * @param[in,out] p_measurement_data A pointer to the pointer of the measurement data structure.
 */
static void FreeBanchmarkData(IPC_CONNECTION_MEASURE_T **p_measurement_data )
{
    if ((*p_measurement_data)->p_msg != NULL)
    {
        free((*p_measurement_data)->p_msg);
    }
    free(*p_measurement_data);
    *p_measurement_data = nullptr;
}


/**
 * @brief Sends data through the specified IPC socket.
 *
 * @param ipc_socket The IPC socket to use for sending data.
 * @param data A pointer to the data to send.
 * @param data_len The length of the data to send in bytes.
 * @return int STATUS_OK if successful, STATUS_FAIL otherwise.
 */
static int SendSocketData(SOCKET ipc_socket, unsigned char *data, size_t data_len)
{
    int sent = 0;
    while (sent < data_len) {
        int socket_result = send(ipc_socket, data+sent, data_len-sent, 0);
        if (socket_result == SOCKET_ERROR) {
            std::cerr << "Error: send failed. number of bytes to send: " << data_len << std::endl;
            return STATUS_FAIL;
        }
        sent += socket_result;
    }
    return STATUS_OK;
}


/**
 * @brief Receives data from the specified socket.
 *
 * @param socket_handle The socket handle to use for receiving data.
 * @return int The number of bytes received if successful, STATUS_FAIL otherwise.
 */
static int ReceiveSocketData(SOCKET socket_handle)
{
	int BytesReceived = 0;
    char buff[SERVER_RESPONSE_SIZE];
    memset(buff, 0, sizeof(buff));

    while (BytesReceived < SERVER_RESPONSE_SIZE) {
        int socket_result = recv(socket_handle, buff+BytesReceived, SERVER_RESPONSE_SIZE-BytesReceived, 0);
        if (socket_result == SOCKET_ERROR) {
            std::cout << "failed to read data from the server" << std::endl;
            return STATUS_FAIL;
        }
        BytesReceived += socket_result;
    }
	
    return BytesReceived;
}

