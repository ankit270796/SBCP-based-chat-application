#include <string>
#include <list>

using namespace std;

#define SBCP_PROTOCOL_VRSN 3

#define SBCP_USERNAME_LEN     16
#define SBCP_MESSAGE_LEN      512
#define SBCP_REASON_LEN       32
#define SBCP_CLIENT_COUNT_LEN 2
#define SBCP_MAX_MSG_LEN      1024

#define SBCP_ATTR_REASON       1
#define SBCP_ATTR_USERNAME     2
#define SBCP_ATTR_CLIENT_COUNT 3
#define SBCP_ATTR_MESSAGE      4

#define SBCP_JOIN    2
#define SBCP_FWD     3
#define SBCP_SEND    4
#define SBCP_NAK     5
#define SBCP_OFFLINE 6
#define SBCP_ACK     7
#define SBCP_ONLINE  8
#define SBCP_IDLE    9

#define SBCP_MSG_HEAD_SIZE  4
#define SBCP_ATTR_HEAD_SIZE 4

#define SBCP_ERR_DISCONNECT        -1
#define SBCP_ERR_RECV              -2
#define SBCP_ERR_HEADER            -3
#define SBCP_ERR_PROTOCOL          -4
#define SBCP_ERR_CORRUPT_HEAD      -5
#define SBCP_ERR_ATTR_HEADER       -6
#define SBCP_ERR_CORRUPT_ATTR_HEAD -7

typedef struct {
	unsigned int type:16;
	unsigned int length:16;
} sbcp_attr_header;

typedef struct {
	unsigned int vrsn:9;
	unsigned int type:7;
	unsigned int length:16;
} sbcp_msg_header;

typedef struct {
	sbcp_attr_header head;
	unsigned int client_count;
	char *payload;
} sbcp_attr;

typedef struct {
	sbcp_msg_header head;
	list<sbcp_attr> payload;
} sbcp_msg;


extern int sbcp_ack(int, int, string);
extern int sbcp_join(int, string);
extern int sbcp_send(int, string);
extern int sbcp_nak(int, string);
extern int sbcp_offline(int, string);
extern int sbcp_online(int, string);
extern int sbcp_recv(int, sbcp_msg*);
extern int sbcp_fwd(int, string, string);