#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include "sbcp.H"
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>

// Thread to manage user input
// Not using select() here
void *get_user_input(void *);

int main(int argc, char const *argv[])
{
	int sd;
	int err;
	struct addrinfo addr_info, *server_info, *it;
	char ip_addr[INET6_ADDRSTRLEN];
	void *client_ip;
	pthread_t handler;
	sbcp_msg fwd;
	list<sbcp_attr>::iterator attr;

	// Check Input Parameters
	if (argc < 4) {
		printf("Invalid usage. Try: \n");
		printf("$ ./echo username server_ip server_port\n");
		return 1;
	}

	// TCP, IPv4/6
	memset(&addr_info, 0, sizeof(addr_info));
	addr_info.ai_family = AF_UNSPEC;
	addr_info.ai_socktype = SOCK_STREAM;

	// Get a Port Number for the IP
	if ((err = getaddrinfo(argv[2], argv[3], &addr_info, &server_info)) != 0) {
		fprintf(stderr, "getaddrinfo() failed: %s\n", gai_strerror(err));
		return 1;
	}

	// Check if we got a port number (In this case, we only have one choice)
	for (it = server_info; it != NULL; it = it->ai_next) {
		sd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
		if (sd == -1) {
			printf("%s:%d ", __FILE__, __LINE__);
			fflush(stdout);
			perror("socket()");
			continue;
		}
		if (connect(sd, it->ai_addr, it->ai_addrlen) == -1) {
			close(sd);
			printf("%s:%d ", __FILE__, __LINE__);
			fflush(stdout);
			perror("connect()");
			continue;
		}
		break;
	}

	if (it == NULL) {
		fprintf(stderr, "Client failed to connect!\n");
		return 2;
	}

	if ((*(struct sockaddr *)it->ai_addr).sa_family == AF_INET) {
		client_ip = &(((struct sockaddr_in *)it->ai_addr)->sin_addr);
	} else {
		client_ip = &(((struct sockaddr_in6 *)it->ai_addr)->sin6_addr);
	}

	inet_ntop(it->ai_family, client_ip, ip_addr, INET6_ADDRSTRLEN);
	printf("Client connecting to %s ...\n", ip_addr);

	freeaddrinfo(server_info);

	// Send JOIN request with username to server
	sbcp_join(sd, argv[1]);
	// Receive ACK
	err = sbcp_recv(sd, &fwd);
	if (err < 0) {
		printf("Error Code: %d\n", err);
		return 1;
	}
	if (fwd.head.type == SBCP_ACK) {
		attr = fwd.payload.begin();
		attr++;
		printf("Current User Count: %s\n", attr->payload);
	}
	else if (fwd.head.type == SBCP_NAK) {
		attr = fwd.payload.begin();
		printf("Connection refused!\nReason: %s\n", attr->payload);
		return 1;
	}
	else {
		printf("Unknown Error!\n");
		return 1;
	}
	// Create a child thread to handle the user inputs
	if (pthread_create(&handler, NULL, get_user_input, (void *)&sd) < 0) {
		printf("%s:%d ", __FILE__, __LINE__);
		fflush(stdout);
		perror("pthread_create()");
		return 1;
	}

	while (1) {
		// Receive messages from server
		err = sbcp_recv(sd, &fwd);
		if (err < 0) {
			printf("Error Code: %d\n", err);
			return 1;
		}
		// If MSG is SBCP_FWD, print on screen
		if (fwd.head.type == SBCP_FWD) {
			attr = fwd.payload.begin();
			printf("%s: ", attr->payload);
			attr++;
			printf("%s\n", attr->payload);
		}
		// If MSG is SBCP_ONLINE, inform user of new user
		if (fwd.head.type == SBCP_ONLINE) {
			attr = fwd.payload.begin();
			printf("User %s is ONLINE\n", attr->payload);
		}
		// If MSG is SBCP_ONLINE, inform user of new user
		if (fwd.head.type == SBCP_OFFLINE) {
			attr = fwd.payload.begin();
			printf("User %s is OFFLINE\n", attr->payload);
		}
	}
	
	close(sd);
	return 0;
}

/*
 * Thread keeps track of user input and sends to server
 */ 
void *get_user_input(void *socket)
{
	int i;
	int sd = *(int *)socket;
	char msg[SBCP_MESSAGE_LEN];
	// Initialize message buffer with 0
	memset(msg, 0, SBCP_MESSAGE_LEN);

	// As user types input, send it to the server
	while(1) {
		fgets(msg, SBCP_MESSAGE_LEN, stdin);
		// If user hits ^D, close the connection
		if (strlen(msg) == 0) {
			close(sd);
			exit(1);
		}
		// Null terminate the input properly
		for (i = 0; i < SBCP_MESSAGE_LEN; i++) {
			if((msg[i] == '\n')||(msg[i] == '\r')) {
				msg[i] = '\0';
				break;
			}
		}
		// Send message to server
		sbcp_send(sd, msg);
		// Clean the message buffer
		memset(msg, 0, SBCP_MESSAGE_LEN);
	}
}