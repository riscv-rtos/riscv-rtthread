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
 * Program List: udp client
 *
 * This is an example routine of a udp client
 * Export the udpclient command to the console
 * Command format: udpclient URL PORT [COUNT = 10]
 * URL: Server address PORT: Port number COUNT: Optional parameter, default is 10
 * Program function: Send COUNT pieces of data to the remote server
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

static const char send_data[] = "This is udp client from RT-Thread.\n"; /* Data used for sending */
static void udpclient(int argc, char **argv)
{
    int sock, port, count;
    struct hostent *host;
    struct sockaddr_in server_addr;
    const char *url;

    if (argc < 3)
    {
        rt_kprintf("Usage: udpclient URL PORT [COUNT = 10]\n");
        rt_kprintf("Like: udpclient 192.168.12.44 5000\n");
        return ;
    }

    url = argv[1];
    port = strtoul(argv[2], 0, 10);

    if (argc > 3)
        count = strtoul(argv[3], 0, 10);
    else
        count = 10;

    /* Obtain the host address through the function entry parameter url (if it is a domain name, it will be resolved) */
    host = (struct hostent *) gethostbyname(url);

    /* Create a socket, type is SOCK_DGRAM, udp type */
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        rt_kprintf("Socket error\n");
        return;
    }

    /* Initialize the pre-connected server address */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr = *((struct in_addr *)host->h_addr);
    rt_memset(&(server_addr.sin_zero), 0, sizeof(server_addr.sin_zero));

    /* Total send count times data */
    while (count)
    {
        /* Send data to the remote server */
        sendto(sock, send_data, strlen(send_data), 0,
               (struct sockaddr *)&server_addr, sizeof(struct sockaddr));

        /* Thread sleep for a while */
        rt_thread_delay(50);

        /* Decrement the count value */
        count --;
    }

    /* Close this socket */
    closesocket(sock);
}

MSH_CMD_EXPORT(udpclient, a udp client sample);
