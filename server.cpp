#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "sbcp.H"

using namespace std;

// A user structure
typedef struct {
	int sd;
	string username;
} user;

// Keeps track of all the users in the system
list<user> clients;

// Signal Handler for Child Process
void sigchld_handler(int signo);
// Thread that handles every client
void *client_handler(void *);

int main(int argc, char const *argv[])
{
	int sd, new_sd;
	int err;
	int yes=1;
	int client_port;
	int bytes_recv;
	struct addrinfo addr_info, *server_info, *it;
	struct sigaction sig;
	struct sockaddr_storage client_addr;
	socklen_t sockaddr_size;
	char ip_addr[INET6_ADDRSTRLEN];
	char *msg, client_msg[SBCP_MESSAGE_LEN];
	void *client_ip;
	pthread_t handler;

	// Populate the address structure
	memset(&addr_info, 0, sizeof(addr_info));
	// IPv4 IPv6 independent
	addr_info.ai_family = AF_UNSPEC;
	// Use TCP
	addr_info.ai_socktype = SOCK_STREAM;
	addr_info.ai_flags = AI_PASSIVE;

	// Check command line arguments
	if (argc < 4) {
		printf("Invalid usage. Try: \n");
		printf("$ ./echos ip_addr port_no max_clients\n");
		return 1;
	}

	if ((err = getaddrinfo(argv[1], argv[2], &addr_info, &server_info)) != 0) {
		fprintf(stderr, "getaddrinfo() failed: %s\n", gai_strerror(err));
		return 1;
	}

	// Iterate through output of getaddrinfo() and find a port to bind to
	for (it = server_info; it != NULL; it = it->ai_next) {
		sd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
		if (sd == -1) {
			printf("%s:%d ", __FILE__, __LINE__);
			fflush(stdout);
			perror("socket()");
			continue;
		}
		if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			printf("%s:%d ", __FILE__, __LINE__);
			fflush(stdout);
			perror("setsockopt()");
			return 1;
		}
		if (bind(sd, it->ai_addr, it->ai_addrlen) == -1) {
			close(sd);
			printf("%s:%d ", __FILE__, __LINE__);
			fflush(stdout);
			perror("bind()");
			// If bind was unsuccesssful, try the next port
			// Note: In this case, there happens to be only one port and
			// so the list server_info only has one element
			continue;
		}
		break;
	}

	freeaddrinfo(server_info);

	// Check if server successfully binded to the given port
	if (it == NULL) {
		fprintf(stderr, "Server failed to bind to given port.\n");
		return 1;
	}

	// Listen on the port
	if (listen(sd, atoi(argv[3])) == -1) {
		printf("%s:%d ", __FILE__, __LINE__);
		fflush(stdout);
		perror("listen()");
		return 1;
	}

	// Register the signal handler for SIGCHLD event
	// so that the resulting process is not a zombie while exiting
	sig.sa_handler = sigchld_handler;
	sigemptyset(&sig.sa_mask);
	sig.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sig, NULL) == -1) {
		printf("%s:%d ", __FILE__, __LINE__);
		fflush(stdout);
		perror("sigaction()");
		return 1;
	}

	printf("Waiting for connections ....\n");

	while (1) {
		sockaddr_size = sizeof(client_addr);
		// Accept incoming connection
		new_sd = accept(sd, (struct sockaddr *)&client_addr, &sockaddr_size);
		if (new_sd == -1) {
			printf("%s:%d ", __FILE__, __LINE__);
			fflush(stdout);
			perror("accept()");
			continue;
		}

		// Print the client IP Address and Port Number
		if ((*(struct sockaddr *)&client_addr).sa_family == AF_INET) {
			client_ip = &(((struct sockaddr_in *)&client_addr)->sin_addr);
		} else {
			client_ip = &(((struct sockaddr_in6 *)&client_addr)->sin6_addr);
		}
		client_port = ntohs((*(struct sockaddr_in *)&client_addr).sin_port);
		inet_ntop(client_addr.ss_family, client_ip, ip_addr, INET6_ADDRSTRLEN);
		printf("Connection accepted from: %s, PORT:%d\n", ip_addr, client_port);

		// Create a child thread to handle the client
		if (pthread_create(&handler, NULL, client_handler, (void *)&new_sd) < 0) {
			printf("%s:%d ", __FILE__, __LINE__);
			fflush(stdout);
			perror("pthread_create()");
			return 1;
		}
	}
}

// SIGCHLD handler
void sigchld_handler(int signo)
{
	int saved_errno = errno;
	while(waitpid(-1, NULL, WNOHANG) > 0);
	errno = saved_errno;
}

// This thread handles each client connected
void *client_handler(void *socket)
{
	int sd = *(int *)socket;
	int err;
	user client;
	sbcp_msg msg;
	sbcp_attr attr;
	string username;
	string message;
	unsigned char client_msg[SBCP_MAX_MSG_LEN];
	list<user>::iterator it;

	// The client always sends a JOIN request first
	err = sbcp_recv(sd, &msg);
	if (msg.head.type != SBCP_JOIN) {
		printf("Client Error!\n");
		fflush(stdout);
		close(sd);
		pthread_exit(NULL);
	}
	// Get username from JOIN request
	attr = *msg.payload.begin();
	username = attr.payload;
	// Check if username exists
	for (it = clients.begin(); it != clients.end(); it++) {
		if (it->username == username) {
			sbcp_nak(sd, "Username already exists!");
			close(sd);
			pthread_exit(NULL);
		}
	}

	// If not, add the user
	client.sd = sd;
	client.username = username;
	clients.push_back(client);
	printf("Client %s added!\n", username.c_str());
	sbcp_ack(sd, clients.size(), username);

	// Also tell the other users about online status
	for (it = clients.begin(); it != clients.end(); it++) {
		if (it->username != username) {
			sbcp_online(it->sd, username);
		}
	}

	// Now listen to Client messages
	while (1) {
		err = sbcp_recv(sd, &msg);
		if (err < 0) {
			break;
		}
		if (msg.head.type == SBCP_SEND) {
			attr = *msg.payload.begin();
			message = attr.payload;
			printf("Received msg from %s: %s\n", username.c_str(), message.c_str());
			for (it = clients.begin(); it != clients.end(); it++) {
				if (it->username != username) {
					sbcp_fwd(it->sd, username, message);
				}
			}
		}
	}

	// If client disconnects, clean up
	if (err == SBCP_ERR_DISCONNECT) {
		printf("Client %s Disconnected!\n", username.c_str());
		fflush(stdout);
	} else {
		printf("%s:%d ", __FILE__, __LINE__);
		printf("Error Code: %d for user %s recv()\n", err, username.c_str());
		fflush(stdout);
	}
	// Remove User
	for (it = clients.begin(); it != clients.end(); it++) {
		if (it->username == username) {
			clients.erase(it);
			break;
		}
	}
	// Also tell other users about offline status
	for (it = clients.begin(); it != clients.end(); it++) {
		sbcp_offline(it->sd, username);
	}
	// Free object
	close(sd);
	pthread_exit(NULL);
}