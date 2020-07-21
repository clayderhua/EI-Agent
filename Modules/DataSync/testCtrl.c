#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <unistd.h>

#define UNIX_SOCKET_NAME "test-socket"
#define MSG_LEN_SIZE 2
#define MSG_VAL_SIZE 128

char g_testBuffer[MSG_VAL_SIZE*2+1]; // key=value
char LenBuffer[MSG_LEN_SIZE];

int writeString(int sock, char* buffer)
{
	char LenBuffer[MSG_LEN_SIZE];
	int msg_len = strlen(buffer);

	LenBuffer[0] = msg_len & 0x00FF;
	LenBuffer[1] = (msg_len & 0xFF00) >> 8;

	// write length
	if (write(sock, LenBuffer, MSG_LEN_SIZE) < 0) {
        perror("writing on stream socket");
		return -1;
	}

	// write data
    if (write(sock, buffer, msg_len) < 0) {
        perror("writing on stream socket");
		return -1;
	}

	return 0;
}

int readString(int msgsock, char* buffer, int buffer_len)
{
	int msg_len, rval;

	if ((rval = read(msgsock, buffer, MSG_LEN_SIZE)) < 0) {
		perror("reading stream message");
		return -1;
	} else if (rval == 0) {
		fprintf(stderr, "Ending connection\n");
		return -1;
	}

	msg_len = (buffer[1] << 8) | buffer[0];
	if (msg_len > buffer_len) {
		fprintf(stderr, "Buffer is not engouth\n");
		return -1;
	}

	memset(buffer, 0, buffer_len);

	if (msg_len == 0) {
		return 0;
	} else if ((rval = read(msgsock, buffer, msg_len)) < 0) {
		perror("reading stream message");
		return -1;
	} else if (rval == 0) {
		fprintf(stderr, "Ending connection\n");
		return -1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
    int sock, msg_len;
    struct sockaddr_un server;
    char buf[1024];
	char* key;
	char* value;

	if (argc < 3) {
		fprintf(stderr, "Usage: %s <socket_name> <key> <value>\n", argv[0]);
		return -1;
	}

	key = argv[2];
	if (argc < 4) {
		if (strcmp(key, "help") == 0) {
			value = "";
		} else {
			fprintf(stderr, "Usage: %s <socket_name> <key> <value>\n", argv[0]);
			return -1;
		}
	} else {
		value = argv[3];
	}

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("opening stream socket");
        return -1;
    }
    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, argv[1]);

    if (connect(sock, (struct sockaddr *) &server, sizeof(struct sockaddr_un)) < 0) {
        close(sock);
        perror("connecting stream socket");
        return -1;
    }

	memset(g_testBuffer, 0, sizeof(g_testBuffer));
	snprintf(g_testBuffer, sizeof(g_testBuffer), "%s=%s", key, value);

	writeString(sock, g_testBuffer);
	readString(sock, g_testBuffer, sizeof(g_testBuffer));

	fprintf(stderr, "%s\n", g_testBuffer);

    close(sock);
}

