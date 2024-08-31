/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date             Author      Notes
 * 2024/06/25     flyingcys    first version
 */
/* Program List: tcp client implemented using select
 *
 * This is an example routine of a tcp client implemented using select
 * Export the tcpclient_select command to the console
 * Command format: tcpclient_select URL PORT
 * URL: Server address PORT: Port number
 * Program function: Use select to monitor whether data has arrived on the socket,
 * If data arrives, receive and display the information sent from the server,
 * Exit the program when receiving information starting with 'q' or 'Q'
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

static const char send_data[] = "This is tcp client from RT-Thread."; /* Data used for sending */
static void tcpclient_select(int argc, char **argv)
{
    int ret;
    char *recv_data;
    struct hostent *host;
    int sock, bytes_received;
    struct sockaddr_in server_addr;
    const char *url;
    int port;
    fd_set readset;
    int i, maxfdp1;

    if (argc < 3)
    {
        rt_kprintf("Usage: tcpclient_select URL PORT\n");
        rt_kprintf("Like: tcpclient_select 192.168.12.44 5000\n");
        return ;
    }

    url = argv[1];
    port = strtoul(argv[2], 0, 10);

    /* Obtain the host address through the function entry parameter url (if it is a domain name, it will be resolved) */
    host = gethostbyname(url);

    /* Allocate a buffer for receiving data */
    recv_data = rt_malloc(EXAMPLE_NETWORK_BUFFER_SIZE);
    if (recv_data == RT_NULL)
    {
        rt_kprintf("No memory\n");
        return;
    }

    /* Create a socket, type is SOCKET_STREAM, tcp type */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        /* Socket creation failed */
        rt_kprintf("Socket error\n");

        /* Release the receiving buffer */
        rt_free(recv_data);
        return;
    }

    /* Initialize the pre-connected server address */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr = *((struct in_addr *)host->h_addr);
    rt_memset(&(server_addr.sin_zero), 0, sizeof(server_addr.sin_zero));

    /* Connect to the server */
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
    {
        /* Connection failed */
        rt_kprintf("Connect fail!\n");
        closesocket(sock);

        /* Release the receiving buffer */
        rt_free(recv_data);
        return;
    }

    /* Get the maximum descriptor number to be monitored */
    maxfdp1 = sock + 1;

    while (1)
    {
        /* Clear the list of readable event descriptors */
        FD_ZERO(&readset);

        /* Add the descriptor that needs to be monitored for readable events to the list */
        FD_SET(sock, &readset);

        /* Wait for the network descriptor to have an event */
        i = select(maxfdp1, &readset, 0, 0, 0);

        /* At least one file descriptor has the specified event, continue running */
        if (i == 0) continue;

        /* Check if there is a readable event on the sock descriptor */
        if (FD_ISSET(sock, &readset))
        {
            /* Receive up to EXAMPLE_NETWORK_BUFFER_SIZE - 1 bytes of data from the sock connection */
            bytes_received = recv(sock, recv_data, EXAMPLE_NETWORK_BUFFER_SIZE - 1, 0);
            if (bytes_received < 0)
            {
                /* Reception failed, close this connection */
                closesocket(sock);
                rt_kprintf("\nreceived error,close the socket.\r\n");

                /* Release the receiving buffer */
                rt_free(recv_data);
                break;
            }
            else if (bytes_received == 0)
            {
                /* The default recv is in blocking mode, receiving 0 means the connection is broken, close this connection */
                closesocket(sock);
                rt_kprintf("\nreceived error,close the socket.\r\n");

                /* Release the receiving buffer */
                rt_free(recv_data);
                break;
            }

            /* Data received, clear the end */
            recv_data[bytes_received] = '\0';

            if (strncmp(recv_data, "q", 1) == 0 || strncmp(recv_data, "Q", 1) == 0)
            {
                /* If the first letter is q or Q, close this connection */
                closesocket(sock);
                rt_kprintf("\n got a 'q' or 'Q',close the socket.\r\n");

                /* Release the receiving buffer */
                rt_free(recv_data);
                break;
            }
            else
            {
                /* Display the received data on the console */
                rt_kprintf("\nReceived data = %s ", recv_data);
            }

            /* Send data to the sock connection */
            ret = send(sock, send_data, strlen(send_data), 0);
            if (ret < 0)
            {
                /* Reception failed, close this connection */
                closesocket(sock);
                rt_kprintf("\nsend error,close the socket.\r\n");

                rt_free(recv_data);
                break;
            }
            else if (ret == 0)
            {
                /* Print a warning message when the return value of the send function is 0 */
                rt_kprintf("\n Send warning,send function return 0.\r\n");
            }
        }
    }
    return;
}

MSH_CMD_EXPORT(tcpclient_select, a tcp client sample by select api);
