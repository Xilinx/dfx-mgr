/*
 * Copyright (c) 2021, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

//#include <libwebsockets.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
//#include <dfx-mgr/accel.h>
//#include <dfx-mgr/assert.h>
//#include <dfx-mgr/shell.h>
//#include <dfx-mgr/model.h>
//#include <dfx-mgr/json-config.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/select.h>
#include <dfx-mgr/dfxmgr_client.h>
#include <dfx-mgr/print.h>
//#include <dfx-mgr/sys/linux/graph/jobScheduler.h>
//#include <dfx-mgr/sys/linux/graph/abstractGraph.h>
#include "utils.h"
#include "graph_api.h"
#include <dfx-mgr/daemon_helper.h>
#define WATCH_PATH_LEN 256
#define MAX_WATCH 50
#define MAX_CLIENTS 200

static volatile int interrupted = 0;
static int socket_d;

void intHandler(int dummy) {
	_unused(dummy);
	interrupted = 1;
	exit(0);
}

void error (char *msg)
{
	perror (msg);
	exit (1);
}

//#define GRAPH_INIT                1
//#define GRAPH_INIT_DONE           11

int main (int argc, char **argv)
{
	struct message recv_message, send_message;
	struct stat statbuf;
	struct sockaddr_un socket_address;
	int slot, ret, size, status;
	char *msg;
	int buff_fd[25];
	int buff_fd_cnt = 0;
	GRAPH_HANDLE gHandle;
	dfx_init();
	GRAPH_MANAGER_HANDLE gmHandle = GraphManager_Create(3);
	GraphManager_startServices(gmHandle);
	int id;

	signal(SIGINT, intHandler);
	_unused(argc);
	_unused(argv);
	_unused(slot);
	_unused(ret);
	_unused(msg);
	// initialize the complaint queue
	//JobScheduler_t *scheduler = jobSchedulerInit();

	if (stat(SERVER_SOCKET, &statbuf) == 0) {
		if (unlink(SERVER_SOCKET) == -1)
			error ("unlink");
	}
    

	if ((socket_d = socket (AF_UNIX, SOCK_SEQPACKET, 0)) == -1)
		error ("socket");


	memset (&socket_address, 0, sizeof (struct sockaddr_un));
	socket_address.sun_family = AF_UNIX;
	strncpy (socket_address.sun_path, SERVER_SOCKET, sizeof(socket_address.sun_path) - 1);

	if (bind (socket_d, (const struct sockaddr *) &socket_address, sizeof (struct sockaddr_un)) == -1)
		error ("bind");

	// Mark socket for accepting incoming connections using accept
	if (listen (socket_d, BACKLOG) == -1)
		error ("listen");

	fd_set fds, readfds;
	FD_ZERO (&fds);
	FD_SET (socket_d, &fds);
	int fdmax = socket_d;

	//acapd_print("dfx-mgr daemon started.\n");
	while (1) {
		readfds = fds;
		// monitor readfds for readiness for reading
		if (select (fdmax + 1, &readfds, NULL, NULL, NULL) == -1)
			error ("select");
        
		// Some sockets are ready. Examine readfds
		for (int fd = 0; fd < (fdmax + 1); fd++) {
			if (FD_ISSET (fd, &readfds)) {  // fd is ready for reading 
				if (fd == socket_d) {  // request for new connection
					int fd_new;
					if ((fd_new = accept (socket_d, NULL, NULL)) == -1)
						error ("accept");

					FD_SET (fd_new, &fds); 
					if (fd_new > fdmax) 
						fdmax = fd_new;
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
						//acapd_debug("Recieved packet with ID : %d\n", recv_message.id);
						switch (recv_message.id) {

							case GRAPH_INIT:
								acapd_debug("daemon recieved GRAPH_INIT");
								gHandle = Graph_CreateWithPriority("test", 2);
								id = Graph_fromJson(gHandle, recv_message.data);
								GraphManager_addGraph(gmHandle, gHandle);
								//printf("%s\n", Graph_toJson(gHandle));
								//buff_fd_cnt = abstractGraphServerConfig(scheduler, 
								//	recv_message.data, recv_message.size, buff_fd);
								//GRAPH_HANDLE gHandle; // = Graph_CreateWithPriority("test", 2);
								//memcpy(buff_fd, "hello", sizeof("hello"));
								buff_fd_cnt = 0;
								memcpy(send_message.data, &id, sizeof(id));					
								send_message.id = GRAPH_INIT_DONE;
								send_message.fdcount = buff_fd_cnt;
								send_message.size = sizeof(id);
								sock_fd_write(fd, &send_message, 
										HEADERSIZE + send_message.size,
										buff_fd, buff_fd_cnt);
								break;
							case GRAPH_STAGED:
								acapd_debug("Recieved graph scheduled packet");
								int * idptr = (int *) recv_message.data;
								id = *idptr;
								int *io_fd, *io_ids, io_size = 0;
								status = GraphManager_ifGraphStaged(gmHandle, id, &io_fd, &io_ids, &io_size);
								if (status == true){
									for(int i = 0 ; i < io_size; i++){
										printf("### id = %x\n", io_ids[i]);
										printf("### fd = %x\n", io_fd[i]);
										buff_fd[i] = io_fd[i];
									}
									//acapd_debug("%d\n", status);
									//acapd_debug("%d\n", io_size);
									size = sizeof(int)* (io_size + 1);
									memcpy((char*)send_message.data, &status, sizeof(int));					
									memcpy((char*)send_message.data + sizeof(int), io_ids, sizeof(int) * io_size);					
									send_message.id = GRAPH_STAGED;
									send_message.fdcount = io_size;
									send_message.size = size;
								}
								else{
									acapd_debug("Not Staged !! %d\n", status);
									size = sizeof(int);
									memcpy(send_message.data, &status, sizeof(int));					
									send_message.id = GRAPH_STAGED;
									send_message.fdcount = 0;
									send_message.size = size;
								}
								sock_fd_write(fd, &send_message, 
										HEADERSIZE + send_message.size,
										buff_fd, io_size);
								break;
							case GRAPH_FINALISE:
								//acapd_debug("daemon recieved graph_finalise\n");
								//abstractGraphServerFinalise(scheduler, recv_message.data);
								memcpy(send_message.data, recv_message.data, 
										recv_message.size);
								send_message.size = recv_message.size;
								//send_message.id = GRAPH_FINALISE_DONE;

								if (write(fd, &send_message, HEADERSIZE +send_message.size) < 0)
									printf("GRAPH_FINALISE write failed\n");
								break;

								/*case LOAD_ACCEL:
								  acapd_print("daemon loading accel %s \n",recv_message.data);
								  slot = load_accelerator(recv_message.data);
								  sprintf(send_message.data, "%d", slot);
								  send_message.size = 2;
								  if (write(fd, &send_message, HEADERSIZE + send_message.size) < 0)
								  acapd_perror("LOAD_ACCEL write failed\n");
								  break;

								  case REMOVE_ACCEL:
								  acapd_print("daemon removing accel at slot %d\n",atoi(recv_message.data));
								  ret = remove_accelerator(atoi(recv_message.data));
								  sprintf(send_message.data, "%d", ret);
								  send_message.size = 2;
								  if (write(fd, &send_message, HEADERSIZE+ send_message.size) < 0)
								  acapd_perror("REMOVE_ACCEL write failed\n");
								  break;

								  case LIST_PACKAGE:
								  msg = listAccelerators();
								  memcpy(send_message.data, msg, sizeof(send_message.data)); 
								  send_message.size = sizeof(send_message.data);
								  if (write(fd, &send_message, send_message.size) < 0)
								  acapd_perror("LIST_PACKAGE: write failed\n");
								  break;*/
								/*case QUIT:
								  if (close (fd) == -1)
								  error ("close");
								  FD_CLR (fd, &fds);
								  break;
								  */
							default: 
								printf("Unexpected message from client\n"); 
						}
					}
				}
			} 
		} 
	} 
	exit (EXIT_SUCCESS);
}