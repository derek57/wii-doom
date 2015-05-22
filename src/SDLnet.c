/*
  SDL_net:  An example cross-platform network library for use with SDL
  Copyright (C) 1997-2013 Sam Lantinga <slouken@libsdl.org>
  Copyright (C) 2012 Simeon Maxein <smaxein@googlemail.com>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/* $Id$ */


#ifdef WITHOUT_SDL
#include <stdarg.h>
#include <string.h>
#endif

#include "c_io.h"
#include "SDL_net.h"
#include "SDLnetsys.h"


//void C_Printf(stringtype_t type, char *s, ...);

const SDLNet_version *SDLNet_Linked_Version(void)
{
    static SDLNet_version linked_version;
    SDL_NET_VERSION(&linked_version);
    return(&linked_version);
}

/* Since the UNIX/Win32/BeOS code is so different from MacOS,
   we'll just have two completely different sections here.
*/
static int SDLNet_started = 0;

#ifndef __USE_W32_SOCKETS
#include <signal.h>
#endif

#ifndef __USE_W32_SOCKETS

int SDLNet_GetLastError(void)
{
    return errno;
}

void SDLNet_SetLastError(int err)
{
    errno = err;
}

#endif

static char errorbuf[1024];

void SDLCALL SDLNet_SetError(const char *fmt, ...)
{
    va_list argp;
    va_start(argp, fmt);
    SDL_vsnprintf(errorbuf, sizeof(errorbuf), fmt, argp);
    va_end(argp);
#ifndef WITHOUT_SDL
    C_Printf(CR_RED, " %s", errorbuf);
    sleep(1);
#endif
}

const char * SDLCALL SDLNet_GetError(void)
{
#ifdef WITHOUT_SDL
    return errorbuf;
#else
    return SDL_GetError();
#endif
}

// >>> FIX: For Nintendo Wii using devkitPPC / libogc
// New variable to signal if_config() error:
int netinit_error;
// <<< FIX

// >>> FIX: For Nintendo Wii using devkitPPC / libogc
// New variable containing the current IP address of the device:
char ipaddress_text[16];
// <<< FIX

// >>> FIX: For Nintendo Wii using devkitPPC / libogc
// Adding stuff supposed to be present on the previously removed headers:
#define MAXHOSTNAMELEN        256
// <<< FIX

#define        NET_NAMELEN 64

char my_tcpip_address[NET_NAMELEN];

static int net_controlsocket;
//static struct sockaddr broadcastaddr;

void I_Error (char *error, ...);

//int SDLNet_UDP_Open(Uint16 port);

/* Initialize/Cleanup the network API */
int  SDLNet_Init(void)
{
/*
    if ( !SDLNet_started ) {
#ifdef __USE_W32_SOCKETS
        // Start up the windows networking
        WORD version_wanted = MAKEWORD(1,1);
        WSADATA wsaData;

        if ( WSAStartup(version_wanted, &wsaData) != 0 ) {
            printf("Couldn't initialize Winsock 1.1\n");
            sleep(1);
            return(-1);
        }
#else
        // SIGPIPE is generated when a remote socket is closed
        void (*handler)(int);
        handler = signal(SIGPIPE, SIG_IGN);
        if ( handler != SIG_DFL ) {
            signal(SIGPIPE, handler);
        }
#endif
    }
    ++SDLNet_started;
    return(0);

        struct sockaddr_in sock_addr;
*/
// >>> FIX: For Nintendo Wii using devkitPPC / libogc
// This variable is not needed in the current implementation:
        //struct hostent *local;
// <<< FIX
        char        buff[MAXHOSTNAMELEN];
// >>> FIX: For Nintendo Wii using devkitPPC / libogc
// This variable is not needed in the current implementation:
        //struct qsockaddr addr;
// <<< FIX
        char *colon;
        
        do
        {
                netinit_error = if_config(ipaddress_text, NULL, NULL, TRUE);
        } while(netinit_error == -EAGAIN);

// >>> FIX: For Nintendo Wii using devkitPPC / libogc
// Signal as uninitialized if if_config() failed previously:
        if(netinit_error < 0)
        {
                C_Printf(CR_BLUE, " SDLNet_Init: if_config() failed with %i", netinit_error);
                sleep(1);
                return -1;
        };
// <<< FIX

        // determine my name & address
// >>> FIX: For Nintendo Wii using devkitPPC / libogc
// Since we don't currently have a gethostname(), or equivalent, function, let's just paste the IP address of the device:
        //gethostname(buff, MAXHOSTNAMELEN);
        //local = gethostbyname(buff);
        //myAddr = *(int *)local->h_addr_list[0];
        strcpy(buff, ipaddress_text);
// <<< FIX

        // if the quake hostname isn't set, set it to the machine name
//        if (strcmp(hostname.string, "UNNAMED") == 0)
        {
//                buff[15] = "hostname";
//                Cvar_Set ("hostname", buff);
        }
/*
        if ((net_controlsocket = SDLNet_UDP_Open (0)) == -1)
                I_Error("UDP_Init: Unable to open control socket\n");
*/
// >>> FIX: For Nintendo Wii using devkitPPC / libogc
// Since we can't bind anything to port 0, the following line does not work. Replacing:
        //UDP_GetSocketAddr (net_controlsocket, &addr);
        //strcpy(my_tcpip_address,  UDP_AddrToString (&addr));
        strcpy(my_tcpip_address, buff);
// <<< FIX
        colon = strrchr (my_tcpip_address, ':');
        if (colon)
                *colon = 0;

        C_Printf(CR_BLUE, " SDLNet_Init: UDP Initialized\n");
        sleep(1);
//        tcpipAvailable = TRUE;

        return net_controlsocket;
}
void SDLNet_Quit(void)
{
    if ( SDLNet_started == 0 ) {
        return;
    }
    if ( --SDLNet_started == 0 ) {
#ifdef __USE_W32_SOCKETS
        /* Clean up windows networking */
        if ( WSACleanup() == SOCKET_ERROR ) {
            if ( WSAGetLastError() == WSAEINPROGRESS ) {
#ifndef _WIN32_WCE
                WSACancelBlockingCall();
#endif
                WSACleanup();
            }
        }
#else
        /* Restore the SIGPIPE handler */
        void (*handler)(int);
        handler = signal(SIGPIPE, SIG_DFL);
        if ( handler != SIG_IGN ) {
            signal(SIGPIPE, handler);
        }
#endif
    }
}

/* Resolve a host name and port to an IP address in network form */
int SDLNet_ResolveHost(IPaddress *address, const char *host, Uint16 port)
{
    int retval = 0;

    /* Perform the actual host resolution */
    if ( host == NULL ) {
        C_Printf(CR_BLUE, " SDLNet_ResolveHost: Host is NULL\n");
        sleep(1);
        address->host = INADDR_ANY;
    } else {
        address->host = inet_addr(host);
        if ( address->host == INADDR_NONE ) {
            struct hostent *hp;

            hp = net_gethostbyname(host);
            if ( hp ) {
                memcpy(&address->host,hp->h_addr,hp->h_length);
            } else {
                C_Printf(CR_BLUE, " SDLNet_ResolveHost: Couldn't resolve host\n");
                sleep(1);
                retval = -1;
            }
        }
    }
    address->port = SDLNet_Read16(&port);

    /* Return the status */
    return(retval);
}

/* Resolve an ip address to a host name in canonical form.
   If the ip couldn't be resolved, this function returns NULL,
   otherwise a pointer to a static buffer containing the hostname
   is returned.  Note that this function is not thread-safe.
*/
/* Written by Miguel Angel Blanch.
 * Main Programmer of Arianne RPG.
 * http://come.to/arianne_rpg
 */
const char *SDLNet_ResolveIP(const IPaddress *ip)
{
/*
    struct hostent *hp;
    struct in_addr in;

    hp = net_gethostbyaddr((const char *)&ip->host, sizeof(ip->host), AF_INET);
    if ( hp != NULL ) {
        return hp->h_name;
    }

    in.s_addr = ip->host;
    return inet_ntoa(in);
*/
    char *name = NULL;

    struct hostent *hp = NULL;

    strcpy (name, (const char *)&ip->host);

    hp->h_name = name;
        return 0;
}

int SDLNet_GetLocalAddresses(IPaddress *addresses, int maxcount)
{
    int count = 0;
#ifdef SIOCGIFCONF
/* Defined on Mac OS X */
#ifndef _SIZEOF_ADDR_IFREQ
#define _SIZEOF_ADDR_IFREQ sizeof
#endif
    SOCKET sock;
    struct ifconf conf;
    char data[4096];
    struct ifreq *ifr;
    struct sockaddr_in *sock_addr;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if ( sock == INVALID_SOCKET ) {
        return 0;
    }

    conf.ifc_len = sizeof(data);
    conf.ifc_buf = (caddr_t) data;
    if ( ioctl(sock, SIOCGIFCONF, &conf) < 0 ) {
        closesocket(sock);
        return 0;
    }

    ifr = (struct ifreq*)data;
    while ((char*)ifr < data+conf.ifc_len) {
        if (ifr->ifr_addr.sa_family == AF_INET) {
            if (count < maxcount) {
                sock_addr = (struct sockaddr_in*)&ifr->ifr_addr;
                addresses[count].host = sock_addr->sin_addr.s_addr;
                addresses[count].port = sock_addr->sin_port;
            }
            ++count;
        }
        ifr = (struct ifreq*)((char*)ifr + _SIZEOF_ADDR_IFREQ(*ifr));
    }
    closesocket(sock);
#elif defined(__WIN32__)
    PIP_ADAPTER_INFO pAdapterInfo;
    PIP_ADAPTER_INFO pAdapter;
    PIP_ADDR_STRING pAddress;
    DWORD dwRetVal = 0;
    ULONG ulOutBufLen = sizeof (IP_ADAPTER_INFO);

    pAdapterInfo = (IP_ADAPTER_INFO *) malloc(sizeof (IP_ADAPTER_INFO));
    if (pAdapterInfo == NULL) {
        return 0;
    }

    if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == ERROR_BUFFER_OVERFLOW) {
        pAdapterInfo = (IP_ADAPTER_INFO *) realloc(pAdapterInfo, ulOutBufLen);
        if (pAdapterInfo == NULL) {
            return 0;
        }
        dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);
    }

    if (dwRetVal == NO_ERROR) {
        for (pAdapter = pAdapterInfo; pAdapter; pAdapter = pAdapter->Next) {
            for (pAddress = &pAdapterInfo->IpAddressList; pAddress; pAddress = pAddress->Next) {
                if (count < maxcount) {
                    addresses[count].host = inet_addr(pAddress->IpAddress.String);
                    addresses[count].port = 0;
                }
                ++count;
            }
        }
    }
    free(pAdapterInfo);
#endif
    return count;
}

#if !defined(WITHOUT_SDL) && !SDL_DATA_ALIGNED /* function versions for binary compatibility */

#undef SDLNet_Write16
#undef SDLNet_Write32
#undef SDLNet_Read16
#undef SDLNet_Read32

/* Write a 16/32 bit value to network packet buffer */
extern DECLSPEC void SDLCALL SDLNet_Write16(Uint16 value, void *area);
extern DECLSPEC void SDLCALL SDLNet_Write32(Uint32 value, void *area);

/* Read a 16/32 bit value from network packet buffer */
extern DECLSPEC Uint16 SDLCALL SDLNet_Read16(void *area);
extern DECLSPEC Uint32 SDLCALL SDLNet_Read32(const void *area);

void  SDLNet_Write16(Uint16 value, void *areap)
{
    (*(Uint16 *)(areap) = SDL_SwapBE16(value));
}

void   SDLNet_Write32(Uint32 value, void *areap)
{
    *(Uint32 *)(areap) = SDL_SwapBE32(value);
}

Uint16 SDLNet_Read16(void *areap)
{
    return (SDL_SwapBE16(*(Uint16 *)(areap)));
}

Uint32 SDLNet_Read32(const void *areap)
{
    return (SDL_SwapBE32(*(Uint32 *)(areap)));
}

#endif /* !defined(WITHOUT_SDL) && !SDL_DATA_ALIGNED */
