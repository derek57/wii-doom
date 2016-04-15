//
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//      Loopback network module for server compiled into the client
//

#ifndef WII
#include <stdio.h>
#include <stdlib.h>

#include "doomfeatures.h"
#include "doomtype.h"
#include "i_system.h"
#include "m_misc.h"
#include "net_defs.h"
#include "net_loop.h"
#include "net_packet.h"


#define MAX_QUEUE_SIZE 16


typedef struct
{
    net_packet_t *packets[MAX_QUEUE_SIZE];
    int head, tail;

} packet_queue_t;


static packet_queue_t client_queue;
static packet_queue_t server_queue;
static net_addr_t client_addr;
static net_addr_t server_addr;


static void QueueInit(packet_queue_t *queue)
{
#ifdef DEBUG_NET
    printf("QueueInit\n");
#endif

    queue->head = queue->tail = 0;
}

static void QueuePush(packet_queue_t *queue, net_packet_t *packet)
{
    int new_tail;

#ifdef DEBUG_NET
    printf("QueuePush\n");
#endif

    new_tail = (queue->tail + 1) % MAX_QUEUE_SIZE;

    if (new_tail == queue->head)
    {
        // queue is full
        return;
    }

    queue->packets[queue->tail] = packet;
    queue->tail = new_tail;
}

static net_packet_t *QueuePop(packet_queue_t *queue)
{
    net_packet_t *packet;
    
#ifdef DEBUG_NET
    printf("QueuePop\n");
#endif

    if (queue->tail == queue->head)
    {
        // queue empty
        return NULL;
    }

    packet = queue->packets[queue->head];
    queue->head = (queue->head + 1) % MAX_QUEUE_SIZE;

    return packet;
}

//-----------------------------------------------------------------------------
//
// Client end code
//
//-----------------------------------------------------------------------------
static dboolean NET_CL_InitClient(void)
{
#ifdef DEBUG_NET
    printf("NET_CL_InitClient\n");
#endif

    QueueInit(&client_queue);

    return true;
}

static dboolean NET_CL_InitServer(void)
{
#ifdef DEBUG_NET
    printf("NET_CL_InitServer\n");
#endif

    I_Error("NET_CL_InitServer: attempted to initialize client pipe end as a server!");
    return false;
}

static void NET_CL_SendPacket(net_addr_t *addr, net_packet_t *packet)
{
#ifdef DEBUG_NET
    printf("NET_CL_SendPacket\n");
#endif

    QueuePush(&server_queue, NET_PacketDup(packet));
}

static dboolean NET_CL_RecvPacket(net_addr_t **addr, net_packet_t **packet)
{
    net_packet_t *popped;

#ifdef DEBUG_NET
    printf("NET_CL_RecvPacket\n");
#endif

    popped = QueuePop(&client_queue);

    if (popped != NULL)
    {
        *packet = popped;
        *addr = &client_addr;
        client_addr.module = &net_loop_client_module;
        
        return true;
    }

    return false;
}

static void NET_CL_AddrToString(net_addr_t *addr, char *buffer, int buffer_len)
{
#ifdef DEBUG_NET
    printf("NET_CL_AddrToString\n");
#endif

    M_snprintf(buffer, buffer_len, "local server");
}

static void NET_CL_FreeAddress(net_addr_t *addr)
{
#ifdef DEBUG_NET
    printf("NET_CL_FreeAddress\n");
#endif
}

static net_addr_t *NET_CL_ResolveAddress(char *address)
{
#ifdef DEBUG_NET
    printf("NET_CL_ResolveAddress\n");
#endif

    if (address == NULL)
    {
        client_addr.module = &net_loop_client_module;

        return &client_addr;
    }
    else
    {
        return NULL;
    }
}

net_module_t net_loop_client_module =
{
    NET_CL_InitClient,
    NET_CL_InitServer,
    NET_CL_SendPacket,
    NET_CL_RecvPacket,
    NET_CL_AddrToString,
    NET_CL_FreeAddress,
    NET_CL_ResolveAddress
};

//-----------------------------------------------------------------------------
//
// Server end code
//
//-----------------------------------------------------------------------------
static dboolean NET_SV_InitClient(void)
{
#ifdef DEBUG_NET
    printf("NET_SV_InitClient\n");
#endif

    I_Error("NET_SV_InitClient: attempted to initialize server pipe end as a client!");
    return false;
}

static dboolean NET_SV_InitServer(void)
{
#ifdef DEBUG_NET
    printf("NET_SV_InitServer\n");
#endif

    QueueInit(&server_queue);

    return true;
}

static void NET_SV_SendPacket(net_addr_t *addr, net_packet_t *packet)
{
#ifdef DEBUG_NET
    printf("NET_SV_SendPacket\n");
#endif

    QueuePush(&client_queue, NET_PacketDup(packet));
}

static dboolean NET_SV_RecvPacket(net_addr_t **addr, net_packet_t **packet)
{
    net_packet_t *popped;

#ifdef DEBUG_NET
    printf("NET_SV_RecvPacket\n");
#endif

    popped = QueuePop(&server_queue);

    if (popped != NULL)
    {
        *packet = popped;
        *addr = &server_addr;
        server_addr.module = &net_loop_server_module;
        
        return true;
    }

    return false;
}

static void NET_SV_AddrToString(net_addr_t *addr, char *buffer, int buffer_len)
{
#ifdef DEBUG_NET
    printf("NET_SV_AddrToString\n");
#endif

    M_snprintf(buffer, buffer_len, "local client");
}

static void NET_SV_FreeAddress(net_addr_t *addr)
{
#ifdef DEBUG_NET
    printf("NET_SV_FreeAddress\n");
#endif
}

static net_addr_t *NET_SV_ResolveAddress(char *address)
{
#ifdef DEBUG_NET
    printf("NET_SV_ResolveAddress\n");
#endif

    if (address == NULL)
    {
        server_addr.module = &net_loop_server_module;
        return &server_addr;
    }
    else
    {
        return NULL;
    }
}

net_module_t net_loop_server_module =
{
    NET_SV_InitClient,
    NET_SV_InitServer,
    NET_SV_SendPacket,
    NET_SV_RecvPacket,
    NET_SV_AddrToString,
    NET_SV_FreeAddress,
    NET_SV_ResolveAddress
};
#endif

