#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "error.h"

int connect_to(const char *host, const int portnr)
{
	struct addrinfo hints = { 0 };
	hints.ai_family    = AF_UNSPEC;    // Allow IPv4 or IPv6
	hints.ai_socktype  = SOCK_STREAM;
	hints.ai_flags     = AI_PASSIVE;    // For wildcard IP address
	hints.ai_protocol  = 0;          // Any protocol
	hints.ai_canonname = nullptr;
	hints.ai_addr      = nullptr;
	hints.ai_next      = nullptr;

	char portnr_str[8] { 0 };
	snprintf(portnr_str, sizeof portnr_str, "%d", portnr);

	struct addrinfo *result = nullptr;
	int rc = getaddrinfo(host, portnr_str, &hints, &result);
	if (rc != 0) 
		error_exit(false, "Problem resolving %s: %s\n", host, gai_strerror(rc));

	for(struct addrinfo *rp = result; rp != nullptr; rp = rp->ai_next) {
		int fd = socket(rp -> ai_family, rp -> ai_socktype, rp -> ai_protocol);
		if (fd == -1)
			continue;

		if (connect(fd, rp -> ai_addr, rp -> ai_addrlen) == 0) {
			freeaddrinfo(result);

			return fd;
		}

		close(fd);
	}

	freeaddrinfo(result);

	return -1;
}

void set_nodelay(int fd)
{
	int on = 1;

	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *) &on, sizeof(int)) == -1)
		error_exit(true, "TCP_NODELAY");
}

void set_keepalive(int fd)
{
	int optval = 1;

	if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof optval) == -1)
		error_exit(true, "SO_KEEPALIVE failed");
}

int WRITE(int fd, const char *whereto, size_t len)
{
        ssize_t cnt=0;

        while(len > 0)
        {
                ssize_t rc = write(fd, whereto, len);
                if (rc <= 0)
                        return rc;

		whereto += rc;
		len -= rc;
		cnt += rc;
	}

	return cnt;
}

ssize_t READ(int fd, uint8_t *whereto, size_t len)
{
	ssize_t cnt=0;

	while(len > 0) {
		ssize_t rc = read(fd, whereto, len);

		if (rc == -1) {
			if (errno == EAGAIN)
				continue;

			return -1;
		}
		else if (rc == 0)
			break;
		else {
			whereto += rc;
			len -= rc;
			cnt += rc;
		}
	}

	return cnt;
}
