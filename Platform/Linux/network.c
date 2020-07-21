       #include "network.h"
#include <sys/ioctl.h>
#include <netdb.h>
#include <fcntl.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#ifndef ANDROID
#include <ifaddrs.h>
#endif

static unsigned char g_cacheMAC[6];
static unsigned char g_dockerMAC[6];

void network_init(void)
{
   //Add code
}

int network_connect(char const * const host_name, unsigned int host_port, socket_handle *network_handle)
{
   int iRet = -1;
   //Add code
   int sock = INVALID_SOCKET;
   struct addrinfo hints;
   struct addrinfo *ainfo, *rp;
   int s;
   int opt;
   if(host_name == NULL) return iRet;
   memset(&hints, 0, sizeof(struct addrinfo));
   hints.ai_family = PF_UNSPEC;
   hints.ai_flags = AI_ADDRCONFIG;
   hints.ai_socktype = SOCK_STREAM;

   s = getaddrinfo(host_name, NULL, &hints, &ainfo);
   if(s)
   {
      return iRet;
   }
   for(rp = ainfo; rp != NULL; rp = rp->ai_next)
   {
      sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if(sock == INVALID_SOCKET) continue;

      if(rp->ai_family == PF_INET)
      {
         ((struct sockaddr_in *)rp->ai_addr)->sin_port = htons(host_port);
      }
      else if(rp->ai_family == PF_INET6)
      {
         ((struct sockaddr_in6 *)rp->ai_addr)->sin6_port = htons(host_port);
      }else{
         continue;
      }
      if(connect(sock, rp->ai_addr, rp->ai_addrlen) != -1)
      {

         break;
      }
      close(sock);
   }
   if(!rp){
      return iRet;
   }
   freeaddrinfo(ainfo);

   opt = fcntl(sock, F_GETFL, 0);
   if(opt == -1 || fcntl(sock, F_SETFL, opt | O_NONBLOCK) == -1)
   {
      close(sock);
      return iRet;
   }

   *network_handle = sock;
   iRet = 0;
   return iRet;
}

network_status_t network_waitsock(socket_handle network_handle, int mode, int timeoutms)
{
   int waitret = 0;
   struct timeval timeout;
   int rc = -1;
   fd_set readfd, writefd;
   if(timeoutms >= 0)
   {
      timeout.tv_sec = timeoutms/1000;
      timeout.tv_usec = (timeoutms%1000) * 1000;
   }
   else
   {
      timeout.tv_sec = 1;
      timeout.tv_usec = 0;
   }

   FD_ZERO(&readfd);
   FD_SET(network_handle, &readfd);
   FD_ZERO(&writefd);

   if (mode & network_waitsock_write)
      FD_SET(network_handle, &writefd);

   rc = select(network_handle+1, &readfd, &writefd, NULL, &timeout);

   if (-1 == rc)
   {
      return network_waitsock_error;
   }

   if (FD_ISSET(network_handle, &readfd))
   {
      waitret |= network_waitsock_read;
   }
   if (FD_ISSET(network_handle, &writefd))
   {
      waitret |= network_waitsock_write;
   }
   if(waitret > 0) return waitret;
   return network_waitsock_timeout;
}

int network_send(socket_handle network_handle, char const * sendbuffer, unsigned int len)
{
   return send(network_handle, (char *)sendbuffer, len, 0);
}

int network_recv(socket_handle network_handle, char * recvbuffer, unsigned int len)
{
   return recv(network_handle, (char *)recvbuffer, (int)len, 0);
}

int app_network_close(socket_handle network_handle)
{
   struct linger ling_opt;
   ling_opt.l_linger = 1;
   ling_opt.l_onoff  = 1;
   setsockopt(network_handle, SOL_SOCKET, SO_LINGER, (char*)&ling_opt, sizeof(ling_opt) );
   return close(network_handle);
}

void network_cleanup(void)
{
   //Add code
}

int network_host_name_get(char * phostname, int size)
{
   int iRet = -1;
   char hostName[256];
   if(phostname == NULL) return iRet;
   network_init();
   iRet = gethostname(hostName, size);
   network_cleanup();
   if(!iRet)
   {
      strcpy(phostname, hostName);
   }
   return iRet;
}

int network_ip_get(char * ipaddr, int size)
{
   int iRet = -1;
   int fd;
   struct ifreq ifr;
   struct ifconf ifc;
   char buf[1024];
   if(!ipaddr) return iRet;
   fd = socket( AF_INET, SOCK_DGRAM, 0 );
   if( fd == -1) return iRet;

   ifc.ifc_len = sizeof(buf);
   ifc.ifc_buf = buf;
   if (ioctl(fd, SIOCGIFCONF, &ifc) == -1)
   {
       close(fd);
       return iRet;
   }
   else
   {
       struct ifreq* it = ifc.ifc_req;
       const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));
       int err = 0;
       for (; it != end; ++it) {
       ifr.ifr_addr.sa_family = AF_INET;
           strcpy(ifr.ifr_name, it->ifr_name);
           if (ioctl(fd, SIOCGIFFLAGS, &ifr) == 0) {
                if (! (ifr.ifr_flags & IFF_LOOPBACK)) { // don't count loopback
                    if (ioctl(fd, SIOCGIFADDR, &ifr) == 0) {
                        err = 1;
                        break;
                    }
                }
            }
            else { fprintf(stderr, "ioctl failed: %d\n", err); }
        }

        if (err)
        {
            sprintf(ipaddr, "%s", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
        }
   }

   iRet = 0;
   close(fd);
   return iRet;
}

int network_ip_list_get(char ipsStr[][16], int n)
{	
   int cnt = 0;
   int fd;
   struct ifreq ifr;
   struct ifconf ifc;
   char buf[1024];
   if(!ipsStr || !n) return cnt;
   fd = socket( AF_INET, SOCK_DGRAM, 0 );
   if( fd == -1) return cnt;

   ifc.ifc_len = sizeof(buf);
   ifc.ifc_buf = buf;
   if (ioctl(fd, SIOCGIFCONF, &ifc) == -1)
   {
       close(fd);
       return cnt;
   }
   else
   {
       struct ifreq* it = ifc.ifc_req;
       const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));
       int err = 0;
       for (; it != end; ++it) {
	      ifr.ifr_addr.sa_family = AF_INET;
          strcpy(ifr.ifr_name, it->ifr_name);
          if (ioctl(fd, SIOCGIFFLAGS, &ifr) == 0) {
             if (! (ifr.ifr_flags & IFF_LOOPBACK)) { // don't count loopback
                if (ioctl(fd, SIOCGIFADDR, &ifr) == 0) {
                   sprintf(ipsStr[cnt], "%s", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
				   cnt++;
                }
             }
          }
          else { fprintf(stderr, "ioctl failed: %d\n", err); }
       }
   }
   close(fd);
   return cnt;
}

// refer to OUI table, https://code.wireshark.org/review/gitweb?p=wireshark.git;a=blob_plain;f=manuf
static int isValidMAC(unsigned char* macAddr)
{
	int i = 0, ret = 1, limit;
	unsigned char emptyMAC[6] = {0};
	unsigned int macInt = (macAddr[2] << 16) | (macAddr[1] << 8) | macAddr[0];
	unsigned int invalidList[] = {
		0x000000,        //No Mac, lo
		0xFFFFFF, //No Mac
		0x888888        //Intel No Mac
	};

	unsigned int vmList[] = {
		0x690500,        //vmware
		0x290C00,        //vmware
		0x565000,        //vmware
		0x141C00,        //vmware
		0x421C00,        //parallels
		0xFF0300,         //microsoft virtual pc
		0x4B0F00,        //virtual iron
		0x3E1600,        //Red Hat xen , Oracle vm , xen source, novell xen
		0x270008,        //virtualbox
		0x000000
	};

	unsigned int dockerList[] = {
		0x4202,
		0x4102
	};

	limit = sizeof(invalidList)/sizeof(unsigned int);
	for (i = 0; i < limit; i++) {
		if (macInt == invalidList[i]) {
			return 0;
		}
	}

	limit = sizeof(vmList)/sizeof(unsigned int);
	for (i = 0; i < limit; i++) {
		if (macInt == vmList[i]) {
			if (memcmp(g_cacheMAC, emptyMAC, sizeof(g_cacheMAC)) == 0) { // no cache
				// cache VM MAC if real mac is not found, we can refer to it.
				memcpy(g_cacheMAC, macAddr, sizeof(g_cacheMAC));
			}
			ret = 0;
		}
	}

	// check docker
	macInt = (macAddr[1] << 8) | macAddr[0];
	limit = sizeof(dockerList)/sizeof(unsigned int);
	for (i = 0; i < limit; i++) {
		if (macInt == dockerList[i]) {
			if (memcmp(g_dockerMAC, emptyMAC, sizeof(g_dockerMAC)) == 0) { // no cache
				// cache docker MAC if real mac is not found, we can refer to it.
				memcpy(g_dockerMAC, macAddr, sizeof(g_dockerMAC));
			}
			ret = 0;
		}
	}

	return ret;
}


int network_mac_get(char * macstr)
{
   int iRet = -1;
   int sock_mac;
   struct ifreq ifr_mac;
   struct ifconf ifc;
   char buf[1024];
   if(!macstr) return iRet;
   sock_mac = socket( AF_INET, SOCK_STREAM, 0 );
   if( sock_mac == -1)return iRet;
   ifc.ifc_len = sizeof(buf);
   ifc.ifc_buf = buf;
   if (ioctl(sock_mac, SIOCGIFCONF, &ifc) == -1)
   {
       close(sock_mac);
       return iRet;
   }
   else
   {
       struct ifreq* it = ifc.ifc_req;
       const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));
       int err = 0;
       for (; it != end; ++it) {
           strcpy(ifr_mac.ifr_name, it->ifr_name);
           if (ioctl(sock_mac, SIOCGIFFLAGS, &ifr_mac) == 0) {
                if (! (ifr_mac.ifr_flags & IFF_LOOPBACK)) { // don't count loopback
                    if (ioctl(sock_mac, SIOCGIFHWADDR, &ifr_mac) == 0) {
                        err = 1;
                        break;
                    }
                }
            }
            else { fprintf(stderr, "ioctl failed: %d\n", err); }
        }

        if (err)
        {
            sprintf(macstr,"%02X:%02X:%02X:%02X:%02X:%02X",
               (unsigned char)ifr_mac.ifr_hwaddr.sa_data[0],
               (unsigned char)ifr_mac.ifr_hwaddr.sa_data[1],
               (unsigned char)ifr_mac.ifr_hwaddr.sa_data[2],
               (unsigned char)ifr_mac.ifr_hwaddr.sa_data[3],
               (unsigned char)ifr_mac.ifr_hwaddr.sa_data[4],
               (unsigned char)ifr_mac.ifr_hwaddr.sa_data[5]);
        }
   }

   iRet = 0;
   close(sock_mac);
   return iRet;
}

int getMAC(char *name, char * macstr, bool filter) {
    FILE *f;
    char buf[128];
    char *line = NULL;
    ssize_t count;
    size_t len = 0;
    int iRet = -1;
    unsigned int addr[6] = {0};
    snprintf(buf, sizeof(buf), "/sys/class/net/%s/address", name);
    f = fopen(buf, "r");
    if(!f) {
        fprintf(stderr, "Error opening:%s\n", buf);
        return iRet;
    }
    count = getline(&line, &len, f);
        fclose(f);
    if (count == -1) {
        fprintf(stderr, "Error opening:%s\n", buf);
        return iRet;
    }
    sscanf(line, "%02X:%02X:%02X:%02X:%02X:%02X\n", &addr[0],&addr[1],&addr[2],&addr[3],&addr[4],&addr[5]);
        if(filter)
        {
        sprintf(macstr,"%02X%02X%02X%02X%02X%02X",
            addr[0],
            addr[1],
            addr[2],
            addr[3],
            addr[4],
            addr[5]);
        }
        else
        {
        sprintf(macstr,"%02X:%02X:%02X:%02X:%02X:%02X",
            addr[0],
            addr[1],
            addr[2],
            addr[3],
            addr[4],
            addr[5]);
        }


    iRet = 0;
    free(line);

    return iRet;
}

int network_mac_get_ex2(char * macstr)
{
   int iRet = -1;
   DIR *dir;
   struct dirent *ptr;
   dir = opendir("/sys/class/net/");

   while((ptr = readdir(dir)) != NULL)
   {
      if(strcmp(ptr->d_name, ".") == 0 ||
         strcmp(ptr->d_name, "..") == 0 ||
         strcmp(ptr->d_name, "lo") == 0)
      {
        continue; // skip
      }

      if(getMAC(ptr->d_name, macstr, 1)!=0)
        continue;
      fprintf(stderr, "%s MAC: %s\n", ptr->d_name, macstr);
      if(strcmp(macstr, "000000000000") == 0)
        continue;
      iRet = 0;
      break;
   }
   closedir(dir);
   return iRet;
}

int network_mac_get_ex(char * macstr)
{
    int iRet = -1;
    int sock_mac;
    struct ifreq ifr_mac;
    struct ifconf ifc;
    char buf[1024];
    unsigned char emptyMAC[6] = {0};
	unsigned char* macPtr = NULL;

   // clean string
    memset(g_cacheMAC, 0, sizeof(g_cacheMAC));
    memset(g_dockerMAC, 0, sizeof(g_dockerMAC));

    iRet = network_mac_get_ex2(macstr);
    if(iRet == 0)
  	return iRet;

    if(!macstr) return iRet;
	// clean string
	macstr[0] = '\0';
    sock_mac = socket( AF_INET, SOCK_STREAM, 0 );
    if( sock_mac == -1) return iRet;

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (ioctl(sock_mac, SIOCGIFCONF, &ifc) == -1)
    {
	    close(sock_mac);
	    return iRet;
    }
    else
    {
        struct ifreq* it = ifc.ifc_req;
        const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));

        for (; it != end; ++it) {
            strcpy(ifr_mac.ifr_name, it->ifr_name);
	 		if (ioctl(sock_mac, SIOCGIFHWADDR, &ifr_mac) == 0 && isValidMAC((unsigned char*) ifr_mac.ifr_hwaddr.sa_data)) {
				sprintf(macstr,"%02X%02X%02X%02X%02X%02X",
							(unsigned char) ifr_mac.ifr_hwaddr.sa_data[0],
							(unsigned char) ifr_mac.ifr_hwaddr.sa_data[1],
							(unsigned char) ifr_mac.ifr_hwaddr.sa_data[2],
							(unsigned char) ifr_mac.ifr_hwaddr.sa_data[3],
							(unsigned char) ifr_mac.ifr_hwaddr.sa_data[4],
							(unsigned char) ifr_mac.ifr_hwaddr.sa_data[5]);
	 			break;
	 		}
         }
    }

	if (macstr[0] == '\0') { // no mac found
		if (memcmp(emptyMAC, g_cacheMAC, sizeof(g_cacheMAC)) != 0) { // has cache
			macPtr = g_cacheMAC;
		} else if (memcmp(emptyMAC, g_dockerMAC, sizeof(g_dockerMAC)) != 0) { // has cache
			macPtr = g_dockerMAC;
		}
		if (macPtr) {
			sprintf(macstr,"%02X%02X%02X%02X%02X%02X",
						(unsigned char) macPtr[0],
						(unsigned char) macPtr[1],
						(unsigned char) macPtr[2],
						(unsigned char) macPtr[3],
						(unsigned char) macPtr[4],
						(unsigned char) macPtr[5]);
		}
	}

	iRet = 0;
	close(sock_mac);
	return iRet;
}

int network_mac_list_get_ex2(char macsStr[][20], int n);
int network_mac_list_get(char macsStr[][20], int n)
{
   int sock_mac;
   struct ifreq ifr_mac;
   struct ifconf ifc;
   char buf[1024];
   int cnt = 0;

   if(!macsStr) return cnt;

   cnt = network_mac_list_get_ex2(macsStr, n);
   if(cnt > 0)
    return cnt;

   sock_mac = socket( AF_INET, SOCK_STREAM, 0 );
   if( sock_mac == -1) return cnt;

   ifc.ifc_len = sizeof(buf);
   ifc.ifc_buf = buf;
   if (ioctl(sock_mac, SIOCGIFCONF, &ifc) == -1)
   {
       close(sock_mac);
       return cnt;
   }
   else
   {
       struct ifreq* it = ifc.ifc_req;
       const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));
       for (; it != end; ++it) {
           strcpy(ifr_mac.ifr_name, it->ifr_name);
           if (ioctl(sock_mac, SIOCGIFFLAGS, &ifr_mac) == 0) {
                if (! (ifr_mac.ifr_flags & IFF_LOOPBACK)) { // don't count loopback
                    if (ioctl(sock_mac, SIOCGIFHWADDR, &ifr_mac) == 0) {
                        sprintf(macsStr[cnt],"%02X:%02X:%02X:%02X:%02X:%02X",
                            (unsigned char)ifr_mac.ifr_hwaddr.sa_data[0],
                            (unsigned char)ifr_mac.ifr_hwaddr.sa_data[1],
                            (unsigned char)ifr_mac.ifr_hwaddr.sa_data[2],
                            (unsigned char)ifr_mac.ifr_hwaddr.sa_data[3],
                            (unsigned char)ifr_mac.ifr_hwaddr.sa_data[4],
                            (unsigned char)ifr_mac.ifr_hwaddr.sa_data[5]);
                        cnt++;
                        if(cnt >= n) break;
                    }
                }
            }
            else { fprintf(stderr, "ioctl failed\n"); }
        }
   }
   close(sock_mac);
   return cnt;
}

int network_mac_list_get_ex2(char macsStr[][20], int n)
{
   int cnt = 0;
   DIR *dir;
   struct dirent *ptr;
   dir = opendir("/sys/class/net/");
   while((ptr = readdir(dir)) != NULL)
   {
      char macStr[18] = {0};
      if(strlen(ptr->d_name)<=2)
        continue;
      if(getMAC(ptr->d_name, macStr, false) !=0)
        continue;
      fprintf(stderr, "%s MAC: %s\n", ptr->d_name, macStr);
      if(strcmp(macStr, "00:00:00:00:00:00") == 0)
        continue;
      strcpy(macsStr[cnt], macStr);
      cnt++;
      if(cnt >= n) break;
   }
   closedir(dir);
   return cnt;
}

int network_local_ip_get(int socket, char* clientip, int size)
{
    struct sockaddr_storage addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    //int res = getpeername(socket, (struct sockaddr *)&addr, &addr_size); //server IP
    int res = getsockname(socket, (struct sockaddr *)&addr, &addr_size);
    if(res == 0)
    {
        struct sockaddr_in *s = (struct sockaddr_in *)&addr;
        inet_ntop(addr.ss_family, &s->sin_addr, clientip, size);
    }
    return res;
}
#ifndef ANDROID
int send_broadcast_packet(uint32_t addr_num, unsigned char* packet, int packet_len)
{
    int iRet;
    struct sockaddr_in addr;
    int sockfd;
    int optVal = 1;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd > 0)
    {
        iRet = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST,(char *)&optVal, sizeof(optVal));

        memset((void*)&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(9);
        addr.sin_addr.s_addr = addr_num;
        iRet = sendto(sockfd, packet, packet_len, 0, (struct sockaddr *)&addr, sizeof(addr));
        if (iRet == -1) {
            fprintf(stderr, "[PowerOnOff] sendto fail, errno=%d\n", errno);
        }
        close(sockfd);
    }

    return 0;
}

int send_local_broadcast_WOL(char * mac, int size)
{
    unsigned char packet[102];
    int i = 0, j = 0;
    struct ifaddrs *ifap, *ifa;
    struct sockaddr_in *sa;
    char addr[16];
    uint32_t loopback, addr_num;
    void* ptr;

    if(size < 6)
        return -1;

    // create WOL content
    for(i=0;i<6;i++) {
        packet[i] = 0xFF;
    }
    for(i=1;i<17;i++) {
        for(j=0;j<6;j++) {
            packet[i*6+j] = mac[j];
        }
    }

    // get localhost address
    inet_pton(AF_INET, "127.0.0.1", &loopback);

    getifaddrs (&ifap);
    for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        if (ifa->ifa_addr->sa_family == AF_INET) {
            sa = (struct sockaddr_in *) ifa->ifa_addr;
            ptr = &sa->sin_addr;
            addr_num = *((unsigned int*)ptr);

            if (loopback != addr_num && // skip loopback address
                (ifa->ifa_flags & IFF_BROADCAST) != 0) // broadcast interface
            {
#if 1 // for debug
                inet_ntop(AF_INET, &sa->sin_addr, addr, sizeof(addr));
                fprintf(stderr, "Interface: %s\tAddress: %s\n", ifa->ifa_name, addr);

                sa = (struct sockaddr_in *) ifa->ifa_netmask;
                inet_ntop(AF_INET, &sa->sin_addr, addr, sizeof(addr));
                fprintf(stderr, "Interface: %s\tNetmask: %s\n", ifa->ifa_name, addr);
#endif
                sa = (struct sockaddr_in *) ifa->ifa_broadaddr;
                ptr = &sa->sin_addr;
                addr_num = *((unsigned int*)ptr);
                send_broadcast_packet((uint32_t) addr_num, packet, sizeof(packet));

                inet_ntop(AF_INET, &sa->sin_addr, addr, sizeof(addr));
                fprintf(stderr, "Interface: %s\tBroadcast: %s\n", ifa->ifa_name, addr);
            }
        }
    }

    freeifaddrs(ifap);

    return 0;
}
#endif

bool network_magic_packet_send(char * mac, int size)
{
    bool bRet = false;
    if(size < 6) return bRet;
    {
        unsigned char packet[102];
        struct sockaddr_in addr;
        int sockfd;
        int i = 0, j = 0;
        int optVal = 1;

        for(i=0;i<6;i++)
        {
            packet[i] = 0xFF;
        }
        for(i=1;i<17;i++)
        {
            for(j=0;j<6;j++)
            {
                packet[i*6+j] = mac[j];
            }
        }

        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if(sockfd > 0)
        {
            int iRet = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST,(int *)&optVal, sizeof(optVal));
            if(iRet == 0)
            {
                memset((void*)&addr, 0, sizeof(addr));
                addr.sin_family = AF_INET;
                addr.sin_port = htons(9);
                addr.sin_addr.s_addr= INADDR_BROADCAST;
                iRet = sendto(sockfd, packet, sizeof(packet), 0, (struct sockaddr *)&addr, sizeof(addr));
                if(iRet != -1) bRet = true;
            }
            close(sockfd);
        }
    }
#ifndef ANDROID
    bRet = (!send_local_broadcast_WOL(mac, size) & bRet);
#endif
    return bRet;
}
