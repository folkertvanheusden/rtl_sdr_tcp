// (C) 2023 by Folkert van Heusden, released under MIT license

int connect_to(const char *host, const int portnr);
void set_nodelay(int fd);
void set_keepalive(int fd);

int WRITE(int fd, const char *whereto, size_t len);
ssize_t READ(int fd, uint8_t *whereto, size_t len);
