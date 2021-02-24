#include <sys/un.h>

typedef struct graphSocket graphSocket_t;
struct graphSocket {
        int sock_fd;
        struct sockaddr_un socket_address;
};

extern int graphClientInit(struct graphSocket* gs);
extern int graphClientSubmit(struct graphSocket* gs, char* json, int size);
//extern int graphClientFinalise(struct graphSocket* gs, int id);
extern int graphClientFinalise(struct graphSocket* gs, char* json, int size);
extern ssize_t sock_fd_write(int sock, void *buf, ssize_t buflen, int fd);
extern ssize_t sock_fd_read(int sock, void *buf, ssize_t bufsize, int *fd);

