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
* Program List: udp server
*
* This is an example of a udp server
* Export the udpserver command to the console
* Command call format: udpserv
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

#ifndef EXAMPLE_NETWORK_UDPSERVER_PORT
#define EXAMPLE_NETWORK_UDPSERVER_PORT  5001
#endif

static void udpserver(int argc, char **argv)
{
    int sock;
    int bytes_read;
    char *recv_data;
    socklen_t addr_len;
    struct sockaddr_in server_addr, client_addr;

    /* Allocate buffer for receiving data */
    recv_data = rt_malloc(EXAMPLE_NETWORK_BUFFER_SIZE);
    if (recv_data == RT_NULL)
    {
        /* Memory allocation failed, return */
        rt_kprintf("No memory\n");
        return;
    }

    /* Create a socket, type is SOCK_DGRAM, udp type */
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        rt_kprintf("Socket error\n");

        /* Release the buffer for receiving data */
        rt_free(recv_data);
        return;
    }

    /* Initialize the server address */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(EXAMPLE_NETWORK_UDPSERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    rt_memset(&(server_addr.sin_zero), 0, sizeof(server_addr.sin_zero));

    /* Bind the socket to the server address */
    if (bind(sock, (struct sockaddr *)&server_addr,
             sizeof(struct sockaddr)) == -1)
    {
        /* Binding address failed */
        rt_kprintf("Bind error\n");

        /* Release the buffer for receiving data */
        rt_free(recv_data);
        return;
    }

    addr_len = sizeof(struct sockaddr);
    rt_kprintf("UDPServer Waiting for client on port %d...\n", EXAMPLE_NETWORK_UDPSERVER_PORT);

    while (1)
    {
        /* Receive up to EXAMPLE_NETWORK_BUFFER_SIZE - 1 bytes of data from sock */
        bytes_read = recvfrom(sock, recv_data, EXAMPLE_NETWORK_BUFFER_SIZE - 1, 0,
                              (struct sockaddr *)&client_addr, &addr_len);
        /* udp is fundamentally unlikely to fail in receiving data, unless a timeout is set */

        recv_data[bytes_read] = '\0'; /* Null-terminate the end */

        /* Output the received data */
        rt_kprintf("\n(%s , %d) said : ", inet_ntoa(client_addr.sin_addr),
                   ntohs(client_addr.sin_port));
        rt_kprintf("%s", recv_data);

        /* If the received data is "exit", exit */
        if (strcmp(recv_data, "exit") == 0)
        {
            closesocket(sock);

            /* Release the buffer for receiving data */
            rt_free(recv_data);
            break;
        }
    }

    return;
}

MSH_CMD_EXPORT(udpserver, a udp server sample);
