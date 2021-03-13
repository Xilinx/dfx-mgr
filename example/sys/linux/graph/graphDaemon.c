/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
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
#include <dfx-mgr/sys/linux/graph/graphServer.h>
#include <dfx-mgr/sys/linux/graph/graphClient.h>
#include <dfx-mgr/sys/linux/graph/jobScheduler.h>
#include <dfx-mgr/sys/linux/graph/abstractGraph.h>
#include <dfx-mgr/sys/linux/graph/layer0/debug.h>


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
	_unused(argc);
	_unused(argv);
	// initialize the complaint queue
	tail = NULL;
	JobScheduler_t *scheduler = jobSchedulerInit();

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
		INFO("While :\n");
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
						switch (recv_message.id) {

							case GRAPH_INIT:
								printf("### GRAPH INIT ###\n");
								//printf ("recieved %s\n", recv_message.data);
								int buff_fd[25];
								int buff_fd_cnt = abstractGraphServerConfig(scheduler, 
									recv_message.data, recv_message.size, buff_fd);

								//INFO("%d\n", buff_fd_cnt);
								for (int i = 0; i < buff_fd_cnt; i++){
									INFO("%d\n", buff_fd[i]);
								}
								send_message.id = GRAPH_INIT_DONE;
								send_message.fdcount = buff_fd_cnt;
								send_message.size = 0;
								size = sock_fd_write(fd, &send_message, 
											HEADERSIZE + send_message.size,
											buff_fd, buff_fd_cnt);
								printf ("wrote %ld\n", size);
								break;
							case GRAPH_FINALISE:
								printf("### GRAPH FINALISE ###\n");
								INFO("%s\n", recv_message.data);
								INFO("%p\n", scheduler);	
								abstractGraphServerFinalise(scheduler, recv_message.data);
								memcpy(send_message.data, recv_message.data, 
									recv_message.size);
								send_message.size = recv_message.size;
								send_message.id = GRAPH_FINALISE_DONE;
								
								if (write (fd, &send_message, HEADERSIZE + 
									send_message.size) == -1)
									error ("write");
								
								printf("### GRAPH FINALISE ###\n");
								break;

							case QUIT:
								if (close (fd) == -1)
									error ("close");
								FD_CLR (fd, &fds);
								break;

							default: fprintf (stderr, "Graph-server: Unexpected message from client\n"); 
						}
					}
				}
			} 
		} 
	} 
    exit (EXIT_SUCCESS);
}


