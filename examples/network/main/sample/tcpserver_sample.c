/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date             Author      Notes
 * 2024/06/25     flyingcys    first version
 */
/*
* Program List: TCP server
*
* This is an example of a TCP server
* Export the tcpserver command to the console
* Command call format: tcpserv
* No parameters
* Program function: Acts as a server to receive and display data sent by the client, and exits the program when receiving "exit"
*/
#include <rtthread.h>
#include <string.h>
#include <stdlib.h>

#if !defined(SAL_USING_POSIX)
#error "Please enable SAL_USING_POSIX!"
#else
#include <sys/time.h>
#include <sys/select.h>
#endif
#include <sys/socket.h> /* socket.h header file is needed when using BSD socket */
#include "netdb.h"

#ifndef EXAMPLE_NETWORK_BUFFER_SIZE
#define EXAMPLE_NETWORK_BUFFER_SIZE   1024
#endif

#ifndef EXAMPLE_NETWORK_TCPSERVER_PORT
#define EXAMPLE_NETWORK_TCPSERVER_PORT  5000
#endif

static const char send_data[] = "This is TCP Server from RT-Thread."; /* Data to be sent */
static void tcpserver(int argc, char **argv)
{
    char *recv_data; /* Pointer for receiving data, will be dynamically allocated later */
    socklen_t sin_size;
    int sock, connected, bytes_received;
    struct sockaddr_in server_addr, client_addr;
    rt_bool_t stop = RT_FALSE; /* Stop flag */
    int ret;

    recv_data = rt_malloc(EXAMPLE_NETWORK_BUFFER_SIZE + 1); /* Allocate buffer for receiving data */
    if (recv_data == RT_NULL)
    {
        rt_kprintf("No memory\n");
        return;
    }

    /* A socket needs to be created before use, specifying SOCK_STREAM for TCP socket */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        /* Error handling for socket creation failure */
        rt_kprintf("Socket error\n");

        /* Release the allocated buffer for receiving data */
        rt_free(recv_data);
        return;
    }

    /* Initialize the server address */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(EXAMPLE_NETWORK_TCPSERVER_PORT); /* Port number the server works on */
    server_addr.sin_addr.s_addr = INADDR_ANY;
    rt_memset(&(server_addr.sin_zero), 0, sizeof(server_addr.sin_zero));

    /* Bind the socket to the server address */
    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
    {
        /* Error handling for binding failure */
        rt_kprintf("Unable to bind\n");

        /* Release the allocated buffer for receiving data */
        rt_free(recv_data);
        return;
    }

    /* Listen on the socket */
    if (listen(sock, 5) == -1)
    {
        rt_kprintf("Listen error\n");

        /* Release the buffer for receiving data */
        rt_free(recv_data);
        return;
    }

    rt_kprintf("\nTCPServer Waiting for client on port %d...\n", EXAMPLE_NETWORK_TCPSERVER_PORT);
    while (stop != RT_TRUE)
    {
        sin_size = sizeof(struct sockaddr_in);

        /* Accept a request for a client connection socket, this function call is blocking */
        connected = accept(sock, (struct sockaddr *)&client_addr, &sin_size);
        /* Returns the socket for a successful connection */
        if (connected < 0)
        {
            rt_kprintf("accept connection failed! errno = %d\n", errno);
            continue;
        }

        /* The client address information is pointed to by client_addr returned by accept */
        rt_kprintf("I got a connection from (%s , %d)\n",
                   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        /* Handle the client connection */
        while (1)
        {
            /* Send data to the connected socket */
            ret = send(connected, send_data, strlen(send_data), 0);
            if (ret < 0)
            {
                /* Send failure, close this connection */
                closesocket(connected);
                rt_kprintf("\nsend error,close the socket.\r\n");
                break;
            }
            else if (ret == 0)
            {
                /* Print warning message when send function returns 0 */
                rt_kprintf("\n Send warning,send function return 0.\r\n");
            }

            /* Receive data from the connected socket, the receive buffer is 1024 in size, but not necessarily 1024 bytes */
            bytes_received = recv(connected, recv_data, EXAMPLE_NETWORK_BUFFER_SIZE, 0);
            if (bytes_received < 0)
            {
                /* Receive failure, close this connected socket */
                closesocket(connected);
                break;
            }
            else if (bytes_received == 0)
            {
                /* Print warning message when recv function returns 0 */
                rt_kprintf("\nReceived warning,recv function return 0.\r\n");
                closesocket(connected);
                break;
            }

            /* Null-terminate the end of received data */
            recv_data[bytes_received] = '\0';
            if (strcmp(recv_data, "q") == 0 || strcmp(recv_data, "Q") == 0)
            {
                /* If the first letter is q or Q, close this connection */
                closesocket(connected);
                break;
            }
            else if (strcmp(recv_data, "exit") == 0)
            {
                /* If received is exit, close the entire server */
                closesocket(connected);
                stop = RT_TRUE;
                break;
            }
            else
            {
                /* Display received data in the console */
                rt_kprintf("RECEIVED DATA = %s \n", recv_data);
            }
        }
    }

    /* Exit the server */
    closesocket(sock);

    /* Release the receiving buffer */
    rt_free(recv_data);

    return ;
}

MSH_CMD_EXPORT(tcpserver, a tcp server sample);
