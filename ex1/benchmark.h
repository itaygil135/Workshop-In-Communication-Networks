#ifndef BENCHMARK_H
#define BENCHMARK_H


//typedef int SOCKET;   		// todo - why it was commnet out for client but not for server?
typedef int SOCKET_DATA_T;
typedef struct sockaddr_in SOCKADDR_IN;
//typedef struct linger LINGER;			// todo - verify it is not needed
typedef struct sockaddr *PSOCKADDR;

//#define INVALID_SOCKET  (SOCKET)(~0)  // todo - why it was commnet out for client but not for server?



#define SOCKET_ERROR            (-1)
#define SOCKET_STARTUP(socket_data) (socket_data = 0)
#define GETSOCKETERRNO() (errno)



#define BENCHMARK_WARM_UP_CYCLES    10
#define BENCHMARK_NUM_OF_MESSAGES   100
#define MESSAGE_SIZE_START 1 // starting message size in bytes
#define MESSAGE_SIZE_END 1048576 // ending message size in bytes
#define MESSAGE_SIZE_MULTIPLIER 2 // multiplier for message size (exponential)
#define SERVER_RESPONSE_SIZE 1



#define STATUS_OK                   0
#define STATUS_FAIL                 1

#endif // BENCHMARK_H

