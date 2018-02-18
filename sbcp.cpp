#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <list>
#include <stdlib.h>
#include <string.h>
#include "sbcp.H"
#include <sys/socket.h>


int sbcp_join(int sd, string username)
{
	sbcp_msg_header header;
	sbcp_attr_header attr_header;

	header.vrsn = SBCP_PROTOCOL_VRSN;
	header.type = SBCP_JOIN;
	header.length = 1;
	attr_header.type = SBCP_ATTR_USERNAME;
	attr_header.length = username.length();
	send(sd, &header, sizeof(header), 0);
	send(sd, &attr_header, sizeof(attr_header), 0);
	send(sd, username.c_str(), username.length(), 0);

	return 0;
}


int sbcp_ack(int sd, int count, string username)
{
	sbcp_msg_header header;
	sbcp_attr_header attr_header;
	char buffer[6];
	string count_s;

	memset(buffer, 0, 6);
	sprintf(buffer, "%d", count);
	count_s = string(buffer);

	header.vrsn = SBCP_PROTOCOL_VRSN;
	header.type = SBCP_ACK;
	header.length = 2;
	attr_header.type = SBCP_ATTR_USERNAME;
	attr_header.length = username.length();
	send(sd, &header, sizeof(header), 0);
	send(sd, &attr_header, sizeof(attr_header), 0);
	send(sd, username.c_str(), username.length(), 0);
	attr_header.type = SBCP_ATTR_CLIENT_COUNT;
	attr_header.length = count_s.length() + 1;
	send(sd, &attr_header, sizeof(attr_header), 0);
	send(sd, count_s.c_str(), count_s.length() + 1, 0);

	return 0;
}

int sbcp_online(int sd, string username)
{
	sbcp_msg_header header;
	sbcp_attr_header attr_header;

	header.vrsn = SBCP_PROTOCOL_VRSN;
	header.type = SBCP_ONLINE;
	header.length = 1;
	attr_header.type = SBCP_ATTR_USERNAME;
	attr_header.length = username.length();
	send(sd, &header, sizeof(header), 0);
	send(sd, &attr_header, sizeof(attr_header), 0);
	send(sd, username.c_str(), username.length(), 0);

	return 0;
}

int sbcp_offline(int sd, string username)
{
	sbcp_msg_header header;
	sbcp_attr_header attr_header;

	header.vrsn = SBCP_PROTOCOL_VRSN;
	header.type = SBCP_OFFLINE;
	header.length = 1;
	attr_header.type = SBCP_ATTR_USERNAME;
	attr_header.length = username.length();
	send(sd, &header, sizeof(header), 0);
	send(sd, &attr_header, sizeof(attr_header), 0);
	send(sd, username.c_str(), username.length(), 0);

	return 0;
}

int sbcp_send(int sd, string message)
{
	sbcp_msg_header header;
	sbcp_attr_header attr_header;

	header.vrsn = SBCP_PROTOCOL_VRSN;
	header.type = SBCP_SEND;
	header.length = 1;
	attr_header.type = SBCP_ATTR_MESSAGE;
	attr_header.length = message.length();
	send(sd, &header, sizeof(header), 0);
	send(sd, &attr_header, sizeof(attr_header), 0);
	send(sd, message.c_str(), message.length(), 0);

	return 0;
}


int sbcp_nak(int sd, string reason)
{
	sbcp_msg_header header;
	sbcp_attr_header attr_header;

	header.vrsn = SBCP_PROTOCOL_VRSN;
	header.type = SBCP_NAK;
	header.length = 1;
	attr_header.type = SBCP_ATTR_REASON;
	attr_header.length = reason.length();
	send(sd, &header, sizeof(header), 0);
	send(sd, &attr_header, sizeof(attr_header), 0);
	send(sd, reason.c_str(), reason.length(), 0);

	return 0;
}


int sbcp_fwd(int sd, string username, string message)
{
	sbcp_msg_header header;
	sbcp_attr_header attr_header;

	header.vrsn = SBCP_PROTOCOL_VRSN;
	header.type = SBCP_FWD;
	header.length = 2;
	attr_header.type = SBCP_ATTR_USERNAME;
	attr_header.length = username.length();
	send(sd, &header, sizeof(header), 0);
	send(sd, &attr_header, sizeof(attr_header), 0);
	send(sd, username.c_str(), username.length(), 0);

	attr_header.type = SBCP_ATTR_MESSAGE;
	attr_header.length = message.length();
	send(sd, &attr_header, sizeof(attr_header), 0);
	send(sd, message.c_str(), message.length(), 0);

	return 0;
}

int sbcp_recv(int sd, sbcp_msg *msg)
{
	int i;
	int bytes;
	char *payload;
	unsigned char buffer[SBCP_MAX_MSG_LEN];
	sbcp_msg_header header;
	sbcp_attr_header attr_header;
	sbcp_attr *attr;

	bytes = recv(sd, buffer, SBCP_MSG_HEAD_SIZE, 0);
	if (bytes == 0) {
		return SBCP_ERR_DISCONNECT;
	}
	if (bytes < 0) {
		return SBCP_ERR_RECV;
	}
	if (bytes != SBCP_MSG_HEAD_SIZE) {
		return SBCP_ERR_HEADER;
	}

	memcpy(&header, buffer, SBCP_MSG_HEAD_SIZE);
	if (header.vrsn != SBCP_PROTOCOL_VRSN) {
		return SBCP_ERR_PROTOCOL;
	}

	// Implement all packet error checking here
	/*
	 * IDLE
	*/
	if (header.type == SBCP_IDLE) {
		msg->payload.clear();
		if (header.length > 1) {
			return SBCP_ERR_CORRUPT_HEAD;
		}
		else if (header.length == 0) {
			msg->head = header;
			return 0;
		} else {
			bytes = recv(sd, buffer, SBCP_ATTR_HEAD_SIZE, 0);
			if (bytes == 0) {
				return SBCP_ERR_DISCONNECT;
			}
			if (bytes < 0) {
				return SBCP_ERR_RECV;
			}
			if (bytes != SBCP_ATTR_HEAD_SIZE) {
				return SBCP_ERR_ATTR_HEADER;
			}
			memcpy(&attr_header, buffer, SBCP_ATTR_HEAD_SIZE);
			if (attr_header.type != SBCP_ATTR_USERNAME) {
				return SBCP_ERR_CORRUPT_ATTR_HEAD;
			}

			attr = (sbcp_attr *)malloc(sizeof(sbcp_attr));
			payload = (char *)malloc(attr_header.length*sizeof(char));
			bytes = recv(sd, buffer, attr_header.length, 0);
			if (bytes == 0) {
				return SBCP_ERR_DISCONNECT;
			}
			if (bytes < 0) {
				return SBCP_ERR_RECV;
			}
			for (i = 0; i < attr_header.length; i++) {
				payload[i] = buffer[i];
			}
			attr->head = attr_header;
			attr->payload = payload;
			msg->head = header;
			msg->payload.push_back(*attr);
			free(attr);
			return 0;
		}
	}
	/*
	 * JOIN
	 */
	if (header.type == SBCP_JOIN) {
		msg->payload.clear();
		if (header.length != 1) {
			return SBCP_ERR_CORRUPT_HEAD;
		} else {
			bytes = recv(sd, buffer, SBCP_ATTR_HEAD_SIZE, 0);
			if (bytes == 0) {
				return SBCP_ERR_DISCONNECT;
			}
			if (bytes < 0) {
				return SBCP_ERR_RECV;
			}
			if (bytes != SBCP_ATTR_HEAD_SIZE) {
				return SBCP_ERR_ATTR_HEADER;
			}
			memcpy(&attr_header, buffer, SBCP_ATTR_HEAD_SIZE);
			if (attr_header.type != SBCP_ATTR_USERNAME) {
				return SBCP_ERR_CORRUPT_ATTR_HEAD;
			}

			attr = (sbcp_attr *)malloc(sizeof(sbcp_attr));
			payload = (char *)malloc(attr_header.length*sizeof(char));
			bytes = recv(sd, buffer, attr_header.length, 0);
			if (bytes == 0) {
				return SBCP_ERR_DISCONNECT;
			}
			if (bytes < 0) {
				return SBCP_ERR_RECV;
			}

			for (i = 0; i < attr_header.length; i++) {
				payload[i] = buffer[i];
			}


			attr->head = attr_header;
			attr->payload = payload;
			msg->head = header;
			msg->payload.push_back(*attr);
			free(attr);
			return 0;
		}
	}
	/*
	 * SEND
	 */
	if (header.type == SBCP_SEND) {
		msg->payload.clear();
		if (header.length != 1) {
			return SBCP_ERR_CORRUPT_HEAD;
		} else {
			bytes = recv(sd, buffer, SBCP_ATTR_HEAD_SIZE, 0);
			if (bytes == 0) {
				return SBCP_ERR_DISCONNECT;
			}
			if (bytes < 0) {
				return SBCP_ERR_RECV;
			}
			if (bytes != SBCP_ATTR_HEAD_SIZE) {
				return SBCP_ERR_ATTR_HEADER;
			}
			memcpy(&attr_header, buffer, SBCP_ATTR_HEAD_SIZE);
			if (attr_header.type != SBCP_ATTR_MESSAGE) {
				return SBCP_ERR_CORRUPT_ATTR_HEAD;
			}

			attr = (sbcp_attr *)malloc(sizeof(sbcp_attr));
			payload = (char *)malloc(attr_header.length*sizeof(char));
			bytes = recv(sd, buffer, attr_header.length, 0);
			if (bytes == 0) {
				return SBCP_ERR_DISCONNECT;
			}
			if (bytes < 0) {
				return SBCP_ERR_RECV;
			}

			for (i = 0; i < attr_header.length; i++) {
				payload[i] = buffer[i];
			}

			attr->head = attr_header;
			attr->payload = payload;
			msg->head = header;
			msg->payload.push_back(*attr);
			free(attr);
			return 0;
		}
	}
	/*
	 * FWD
	 */
	if (header.type == SBCP_FWD) {
		msg->payload.clear();
		if (header.length != 2) {
			return SBCP_ERR_CORRUPT_HEAD;
		} else {
			bytes = recv(sd, buffer, SBCP_ATTR_HEAD_SIZE, 0);
			if (bytes == 0) {
				return SBCP_ERR_DISCONNECT;
			}
			if (bytes < 0) {
				return SBCP_ERR_RECV;
			}
			if (bytes != SBCP_ATTR_HEAD_SIZE) {
				return SBCP_ERR_ATTR_HEADER;
			}
			memcpy(&attr_header, buffer, SBCP_ATTR_HEAD_SIZE);
			if (attr_header.type != SBCP_ATTR_USERNAME) {
				return SBCP_ERR_CORRUPT_ATTR_HEAD;
			}

			attr = (sbcp_attr *)malloc(sizeof(sbcp_attr));
			payload = (char *)malloc(attr_header.length*sizeof(char));
			bytes = recv(sd, buffer, attr_header.length, 0);
			if (bytes == 0) {
				return SBCP_ERR_DISCONNECT;
			}
			if (bytes < 0) {
				return SBCP_ERR_RECV;
			}

			for (i = 0; i < attr_header.length; i++) {
				payload[i] = buffer[i];
			}


			attr->head = attr_header;
			attr->payload = payload;
			msg->head = header;
			msg->payload.push_back(*attr);
			free(attr);

			bytes = recv(sd, buffer, SBCP_ATTR_HEAD_SIZE, 0);

			if (bytes == 0) {
				return SBCP_ERR_DISCONNECT;
			}
			if (bytes < 0) {
				return SBCP_ERR_RECV;
			}
			if (bytes != SBCP_ATTR_HEAD_SIZE) {
				return SBCP_ERR_ATTR_HEADER;
			}
			memcpy(&attr_header, buffer, SBCP_ATTR_HEAD_SIZE);
			if (attr_header.type != SBCP_ATTR_MESSAGE) {
				return SBCP_ERR_CORRUPT_ATTR_HEAD;
			}

			attr = (sbcp_attr *)malloc(sizeof(sbcp_attr));
			payload = (char *)malloc(attr_header.length*sizeof(char));
			bytes = recv(sd, buffer, attr_header.length, 0);

			if (bytes == 0) {
				return SBCP_ERR_DISCONNECT;
			}
			if (bytes < 0) {
				return SBCP_ERR_RECV;
			}

			for (i = 0; i < attr_header.length; i++) {
				payload[i] = buffer[i];
			}

			attr->head = attr_header;
			attr->payload = payload;
			msg->head = header;
			msg->payload.push_back(*attr);
			free(attr);
			return 0;
		}
	}
	/*
	 * ONLINE
	 */
	if (header.type == SBCP_ONLINE) {
		msg->payload.clear();
		if (header.length != 1) {
			return SBCP_ERR_CORRUPT_HEAD;
		} else {
			bytes = recv(sd, buffer, SBCP_ATTR_HEAD_SIZE, 0);
			if (bytes == 0) {
				return SBCP_ERR_DISCONNECT;
			}
			if (bytes < 0) {
				return SBCP_ERR_RECV;
			}
			if (bytes != SBCP_ATTR_HEAD_SIZE) {
				return SBCP_ERR_ATTR_HEADER;
			}
			memcpy(&attr_header, buffer, SBCP_ATTR_HEAD_SIZE);
			if (attr_header.type != SBCP_ATTR_USERNAME) {
				return SBCP_ERR_CORRUPT_ATTR_HEAD;
			}

			attr = (sbcp_attr *)malloc(sizeof(sbcp_attr));
			payload = (char *)malloc(attr_header.length*sizeof(char));
			bytes = recv(sd, buffer, attr_header.length, 0);
			if (bytes == 0) {
				return SBCP_ERR_DISCONNECT;
			}
			if (bytes < 0) {
				return SBCP_ERR_RECV;
			}

			for (i = 0; i < attr_header.length; i++) {
				payload[i] = buffer[i];
			}

			attr->head = attr_header;
			attr->payload = payload;
			msg->head = header;
			msg->payload.push_back(*attr);
			free(attr);
			return 0;
		}
	}
	/*
	 * OFFLINE
	 */
	if (header.type == SBCP_OFFLINE) {
		msg->payload.clear();
		if (header.length != 1) {
			return SBCP_ERR_CORRUPT_HEAD;
		} else {
			bytes = recv(sd, buffer, SBCP_ATTR_HEAD_SIZE, 0);
			if (bytes == 0) {
				return SBCP_ERR_DISCONNECT;
			}
			if (bytes < 0) {
				return SBCP_ERR_RECV;
			}
			if (bytes != SBCP_ATTR_HEAD_SIZE) {
				return SBCP_ERR_ATTR_HEADER;
			}
			memcpy(&attr_header, buffer, SBCP_ATTR_HEAD_SIZE);
			if (attr_header.type != SBCP_ATTR_USERNAME) {
				return SBCP_ERR_CORRUPT_ATTR_HEAD;
			}

			attr = (sbcp_attr *)malloc(sizeof(sbcp_attr));
			payload = (char *)malloc(attr_header.length*sizeof(char));
			bytes = recv(sd, buffer, attr_header.length, 0);
			if (bytes == 0) {
				return SBCP_ERR_DISCONNECT;
			}
			if (bytes < 0) {
				return SBCP_ERR_RECV;
			}

			for (i = 0; i < attr_header.length; i++) {
				payload[i] = buffer[i];
			}

			attr->head = attr_header;
			attr->payload = payload;
			msg->head = header;
			msg->payload.push_back(*attr);
			free(attr);
			return 0;
		}
	}
	/*
	 * NAK
	 */
	if (header.type == SBCP_NAK) {
		msg->payload.clear();
		if (header.length != 1) {
			return SBCP_ERR_CORRUPT_HEAD;
		} else {
			bytes = recv(sd, buffer, SBCP_ATTR_HEAD_SIZE, 0);
			if (bytes == 0) {
				return SBCP_ERR_DISCONNECT;
			}
			if (bytes < 0) {
				return SBCP_ERR_RECV;
			}
			if (bytes != SBCP_ATTR_HEAD_SIZE) {
				return SBCP_ERR_ATTR_HEADER;
			}
			memcpy(&attr_header, buffer, SBCP_ATTR_HEAD_SIZE);
			if (attr_header.type != SBCP_ATTR_REASON) {
				return SBCP_ERR_CORRUPT_ATTR_HEAD;
			}

			attr = (sbcp_attr *)malloc(sizeof(sbcp_attr));
			payload = (char *)malloc(attr_header.length*sizeof(char));
			bytes = recv(sd, buffer, attr_header.length, 0);
			if (bytes == 0) {
				return SBCP_ERR_DISCONNECT;
			}
			if (bytes < 0) {
				return SBCP_ERR_RECV;
			}

			for (i = 0; i < attr_header.length; i++) {
				payload[i] = buffer[i];
			}

			attr->head = attr_header;
			attr->payload = payload;
			msg->head = header;
			msg->payload.push_back(*attr);
			free(attr);
			return 0;
		}
	}
	/*
	 * ACK
	 */
	if (header.type == SBCP_ACK) {
		msg->payload.clear();
		if (header.length != 2) {
			return SBCP_ERR_CORRUPT_HEAD;
		} else {
			bytes = recv(sd, buffer, SBCP_ATTR_HEAD_SIZE, 0);
			if (bytes == 0) {
				return SBCP_ERR_DISCONNECT;
			}
			if (bytes < 0) {
				return SBCP_ERR_RECV;
			}
			if (bytes != SBCP_ATTR_HEAD_SIZE) {
				return SBCP_ERR_ATTR_HEADER;
			}
			memcpy(&attr_header, buffer, SBCP_ATTR_HEAD_SIZE);
			if (attr_header.type != SBCP_ATTR_USERNAME) {
				return SBCP_ERR_CORRUPT_ATTR_HEAD;
			}

			attr = (sbcp_attr *)malloc(sizeof(sbcp_attr));
			payload = (char *)malloc(attr_header.length*sizeof(char));
			bytes = recv(sd, buffer, attr_header.length, 0);
			if (bytes == 0) {
				return SBCP_ERR_DISCONNECT;
			}
			if (bytes < 0) {
				return SBCP_ERR_RECV;
			}

			for (i = 0; i < attr_header.length; i++) {
				payload[i] = buffer[i];
			}


			attr->head = attr_header;
			attr->payload = payload;
			msg->head = header;
			msg->payload.push_back(*attr);
			free(attr);

			bytes = recv(sd, buffer, SBCP_ATTR_HEAD_SIZE, 0);

			if (bytes == 0) {
				return SBCP_ERR_DISCONNECT;
			}
			if (bytes < 0) {
				return SBCP_ERR_RECV;
			}
			if (bytes != SBCP_ATTR_HEAD_SIZE) {
				return SBCP_ERR_ATTR_HEADER;
			}
			memcpy(&attr_header, buffer, SBCP_ATTR_HEAD_SIZE);
			if (attr_header.type != SBCP_ATTR_CLIENT_COUNT) {
				return SBCP_ERR_CORRUPT_ATTR_HEAD;
			}

			attr = (sbcp_attr *)malloc(sizeof(sbcp_attr));
			payload = (char *)malloc(attr_header.length*sizeof(char));
			bytes = recv(sd, buffer, attr_header.length, 0);

			if (bytes == 0) {
				return SBCP_ERR_DISCONNECT;
			}
			if (bytes < 0) {
				return SBCP_ERR_RECV;
			}

			for (i = 0; i < attr_header.length; i++) {
				payload[i] = buffer[i];
			}

			attr->head = attr_header;
			attr->payload = payload;
			msg->head = header;
			msg->payload.push_back(*attr);
			free(attr);
			return 0;
		}
	}

}

