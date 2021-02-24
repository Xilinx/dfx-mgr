/* 
 *    Graph-server.c: 
 *
 *
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/select.h>
#include <sys/stat.h>
#include "graphServer.h"
#include "graphClient.h"
#include "abstractGraph.h"
#include "debug.h"

#define MAX_CLIENTS 200

struct element *tail;

struct message recv_message, send_message;

void error (char *msg)
{
	perror (msg);
	exit (1);
}


int main (int argc, char **argv)
{
	ssize_t size;
	printf ("Graph-server: Hello, World!\n");
	// initialize the complaint queue
	tail = NULL;
	Element_t *GraphList = NULL;

	// create a unix domain socket, GRAPH_SOCKET
	// unlink, if already exists
	struct stat statbuf;
	if (stat (GRAPH_SOCKET, &statbuf) == 0) {
		if (unlink (GRAPH_SOCKET) == -1)
			error ("unlink");
	}
    
	int listener;

	if ((listener = socket (AF_UNIX, SOCK_SEQPACKET, 0)) == -1)
		error ("socket");

	struct sockaddr_un socket_address;

	memset (&socket_address, 0, sizeof (struct sockaddr_un));
	socket_address.sun_family = AF_UNIX;
	strncpy (socket_address.sun_path, GRAPH_SOCKET, sizeof(socket_address.sun_path) - 1);

	if (bind (listener, (const struct sockaddr *) &socket_address, sizeof (struct sockaddr_un)) == -1)
		error ("bind");

	// Mark socket for accepting incoming connections using accept
	if (listen (listener, BACKLOG) == -1)
		error ("listen");

	fd_set fds, readfds;
	FD_ZERO (&fds);
	FD_SET (listener, &fds);
	int fdmax = listener;

	printf ("Graph-server: Waiting for a message from a client.\n");
	while (1) {
		readfds = fds;
		// monitor readfds for readiness for reading
		if (select (fdmax + 1, &readfds, NULL, NULL, NULL) == -1)
			error ("select");
        
		// Some sockets are ready. Examine readfds
		for (int fd = 0; fd < (fdmax + 1); fd++) {
			if (FD_ISSET (fd, &readfds)) {  // fd is ready for reading 
				if (fd == listener) {  // request for new connection
					int fd_new;
					if ((fd_new = accept (listener, NULL, NULL)) == -1)
						error ("accept");

					FD_SET (fd_new, &fds); 
					if (fd_new > fdmax) 
						fdmax = fd_new;
					fprintf (stderr, "Graph-server: new client\n");
				}
				else  // data from an existing connection, receive it
				{
					memset (&recv_message, '\0', sizeof (struct message));
					ssize_t numbytes = read (fd, &recv_message, sizeof (struct message));
					INFO("%d\n", numbytes);
					INFO("%d\n", recv_message.id);
					if (numbytes == -1)
						error ("read");
					else if (numbytes == 0) {
						// connection closed by client
						fprintf (stderr, "Socket %d closed by client\n", fd);
						if (close (fd) == -1)
						error ("close");
						FD_CLR (fd, &fds);
					}
			                else 
                   			{
						// data from client
						memset (&send_message, '\0', sizeof (struct message));
						INFO("%d\n", recv_message.id);
						switch (recv_message.id) {

							case GRAPH_INIT:
								printf("### GRAPH INIT ###\n");
								//printf ("recieved %s\n", recv_message.data);
								abstractGraphServerConfig(&GraphList, 
									recv_message.data, recv_message.size);

								send_message.id = GRAPH_INIT_DONE;
								size = sock_fd_write(fd, "Hello hh", 8, 1);
								printf ("wrote %ld\n", size);
								break;
							case GRAPH_FINALISE:
								printf("### GRAPH FINALISE ###\n");
								memcpy(send_message.data, recv_message.data, 
									recv_message.size);
								send_message.size = recv_message.size;
								send_message.id = GRAPH_FINALISE_DONE;
								if (write (fd, &send_message, sizeof(uint32_t) * 2 + 
									send_message.size) == -1)
									error ("write");
								break;
							/*case LOG_COMPLAINT:
							add_to_complaint_q (recv_message.apartment_id, recv_message.remarks, send_message.apartment_id);
							send_message.message_id = COMPLAINT_ADDED;
							if (write (fd, &send_message, sizeof (struct message)) == -1)
							error ("write");
							break;

							case GIVE_ME_A_COMPLAINT: 
							if (give_next_complaint (send_message.apartment_id, send_message.remarks) == -1) {
							// error: No more complaints
							send_message.message_id = NO_MORE_COMPLAINTS;
							if (write (fd, &send_message, sizeof (struct message)) == -1)
							error ("write");
							}
							else
							{
							send_message.message_id = NEXT_COMPLAINT;
							if (write (fd, &send_message, sizeof (struct message)) == -1)
							error ("write");

							}
							break;

							case GIVE_COMPLAINT4APT:
							if (give_complaint (recv_message.apartment_id, send_message.apartment_id, send_message.remarks) == -1) {
							// no complaint for this apartment
							send_message.message_id = NO_COMPLAINT4THIS_APT;
							if (write (fd, &send_message, sizeof (struct message)) == -1)
							error ("write");
							}
							else
							{
							send_message.message_id = NEXT_COMPLAINT;
							if (write (fd, &send_message, sizeof (struct message)) == -1)
							error ("write");
							}
							break;

							case RESOLVE_COMPLAINT:
							if (del_complaint (recv_message.apartment_id, send_message.apartment_id) == -1) {
							// error: No complaint found for this apartment
							send_message.message_id = NO_COMPLAINT4THIS_APT;
							if (write (fd, &send_message, sizeof (struct message)) == -1)
							error ("write");
							}
							else
							{
							// complaint deleted
							send_message.message_id = COMPLAINT_DELETED;
							if (write (fd, &send_message, sizeof (struct message)) == -1)
							error ("write");
							}
							break;*/

							case QUIT:
								if (close (fd) == -1)
									error ("close");
								FD_CLR (fd, &fds);
								break;

							default: fprintf (stderr, "Graph-server: Unexpected message from client\n"); 
						}
					}
				}
			} // if (fd == ...
		} // for
	} // while (1)
    exit (EXIT_SUCCESS);
} // main


