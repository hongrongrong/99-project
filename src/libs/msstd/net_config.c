#include "msstd.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <errno.h>
#include <paths.h>
#include <sys/syscall.h>
#include <sys/resource.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <sys/types.h>    
#include <sys/socket.h>      
#include <net/route.h>  
#include "msdefs.h"

#define PROCNET_ROUTE_PATH		"/proc/net/route"
#define PROCNET_IFINET6_PATH	"/proc/net/if_inet6"
#define PROCNET_ROUTE_IPV6_PATH "/proc/net/ipv6_route"
#define SCOPE_LINK_STR			"fe80"

//hrz.milesight type:0=dhcp 1=router advertisement
int net_get_ipv6_ifaddr(char type, const char *ifname, char *addr, int length, char *prefix, int length2)
{
	int ret = 0;
	FILE *fp;
	char addr6[46] = {0}, dev_name[20] = {0};
	char addr6p[8][5];
	int length_of_prefix, scope_value, if_flags, if_index;
	struct sockaddr_in6 sap = {0};
	
	if (!ifname || !addr || length <= 0 || !prefix || length2 <= 0)
		return -1;

	fp = fopen(PROCNET_IFINET6_PATH, "r");
	if (!fp) return -1;

	lockf(fileno(fp), F_LOCK, 0);	
	
	while (fscanf(fp, "%4s%4s%4s%4s%4s%4s%4s%4s %02x %02x %02x %02x %19s\n",
		addr6p[0], addr6p[1], addr6p[2], addr6p[3],
		addr6p[4], addr6p[5], addr6p[6], addr6p[7],
		 &if_index, &length_of_prefix, &scope_value, &if_flags, dev_name) != EOF) {
		if (!strcmp(dev_name, ifname)) {
			sprintf(addr6, "%s:%s:%s:%s:%s:%s:%s:%s",
				addr6p[0], addr6p[1], addr6p[2], addr6p[3],
				addr6p[4], addr6p[5], addr6p[6], addr6p[7]);
			if (strncmp(addr6p[0], SCOPE_LINK_STR, strlen(SCOPE_LINK_STR)) == 0) {
				//fe80 Scope:Link
				if (type == 1) {
					inet_pton(AF_INET6, addr6, &sap.sin6_addr);
					inet_ntop(AF_INET6, &sap.sin6_addr, addr, length);
					snprintf(prefix, length2, "%d", length_of_prefix);
					lockf(fileno(fp), F_ULOCK, 0);
					fclose(fp);
					return 0;
				}
			} else {
				//2001 Scope:Global
				if (type == 0) {
					inet_pton(AF_INET6, addr6, &sap.sin6_addr);
					inet_ntop(AF_INET6, &sap.sin6_addr, addr, length);
					snprintf(prefix, length2, "%d", length_of_prefix);
					lockf(fileno(fp), F_ULOCK, 0);
					fclose(fp);
					return 0;
				}
			}
		}
	}
	lockf(fileno(fp), F_ULOCK, 0);	
	fclose(fp);
	return -1;
}

int net_get_ipv6_gateway(char type, const char *ifname, char *gateway, int length)
{
	FILE *fp;
	char buffer[80] = {'\0'};
	char path[128] = {'\0'};
	char flag = 0;
	char *dfstr = "default via ";
	char *p, *q;
	
	if (!ifname || !gateway || length <= 0)
		return -1;

	//snprintf(path, sizeof(path), "/bin/ip -6 route show dev %s", ifname);
	snprintf(path, sizeof(path), "ip -6 route show dev %s", ifname);//hi3798 have not /bin/ip
	fp = popen(path, "r");
	if (!fp) return -1;

	lockf(fileno(fp), F_LOCK, 0);
	while (fgets(buffer, sizeof(buffer), fp)) {
		if (!strncmp(buffer, dfstr, strlen(dfstr))) {
			p = (char *)&buffer[strlen(dfstr)];
			if (p) {
				q = strchr(p, ' ');
				if (q) {
					if (type == 1) {
						if (strncmp(p, SCOPE_LINK_STR, strlen(SCOPE_LINK_STR)) == 0) {
							snprintf(gateway, (int)(q - p + 1), "%s", p);
							lockf(fileno(fp), F_ULOCK, 0);
							pclose(fp);
							return 0;
						}
					} else {
						snprintf(gateway, (int)(q - p + 1), "%s", p);
						lockf(fileno(fp), F_ULOCK, 0);
						pclose(fp);
						return 0;
					}
				}
			}
		}
	}

	lockf(fileno(fp), F_ULOCK, 0);
	pclose(fp);
	return -1;
}

int net_get_ipv6_gateway2(char type, const char *ifname, char *addr, int length)
{
	FILE *fp;
	char addr6[46] = {0};
	char tmpAdd6[8][5];
	char tmpOther[9][64];
	struct sockaddr_in6 sap = {0};
	
	if (!ifname || !addr || length <= 0)
		return -1;

	fp = fopen(PROCNET_ROUTE_IPV6_PATH, "r");
	if (!fp) return -1;
	
	lockf(fileno(fp), F_LOCK, 0);
	while (fscanf(fp, "%63s %02s %63s %63s %4s%4s%4s%4s%4s%4s%4s%4s %63s %63s %63s %63s %63s\n",
		tmpOther[0], tmpOther[1], tmpOther[2], tmpOther[3],
		tmpAdd6[0], tmpAdd6[1], tmpAdd6[2], tmpAdd6[3],
		tmpAdd6[4], tmpAdd6[5], tmpAdd6[6], tmpAdd6[7],
		tmpOther[4], tmpOther[5], tmpOther[6], tmpOther[7], tmpOther[8]) != EOF) {
		
		if (!strcmp(tmpOther[8], ifname) && atoi(tmpOther[1]) == 0) {
			sprintf(addr6, "%s:%s:%s:%s:%s:%s:%s:%s",
				tmpAdd6[0], tmpAdd6[1], tmpAdd6[2], tmpAdd6[3],
				tmpAdd6[4], tmpAdd6[5], tmpAdd6[6], tmpAdd6[7]);
			if (strncmp(tmpAdd6[0], SCOPE_LINK_STR, strlen(SCOPE_LINK_STR)) == 0) {
				//fe80 Scope:Link
				if (type == 1) {
					inet_pton(AF_INET6, addr6, &sap.sin6_addr);
					inet_ntop(AF_INET6, &sap.sin6_addr, addr, length);
					lockf(fileno(fp), F_ULOCK, 0);
					fclose(fp);
					return 0;
				}
			} else {
				//2001 Scope:Global
				if (type == 0) {
					inet_pton(AF_INET6, addr6, &sap.sin6_addr);
					inet_ntop(AF_INET6, &sap.sin6_addr, addr, length);
					lockf(fileno(fp), F_ULOCK, 0);
					fclose(fp);
					return 0;
				}
			}
		}
	}
	lockf(fileno(fp), F_ULOCK, 0);
	fclose(fp);
	return -1;
}


int net_is_validipv6(const char *hostname)
{
	struct sockaddr_in6 addr;
	
	if (!hostname) return -1;
	if (strchr(hostname, '.')) return -1;//暂时排除::ffff:204.152.189.116
	if (inet_pton(AF_INET6, hostname, &addr.sin6_addr) != 1) return -1;
    	
	return 0;
}

//1=has ipv6 0=not\error
int net_is_haveipv6(void)
{
	FILE *fp;
	char addr6[46] = {0}, dev_name[20] = {0};
	char addr6p[8][5];
	int length_of_prefix, scope_value, if_flags, if_index;
	struct sockaddr_in6 sap = {0};
	
	fp = fopen(PROCNET_IFINET6_PATH, "r");
	if (!fp) return 0;

	lockf(fileno(fp), F_LOCK, 0);
	while (fscanf(fp, "%4s%4s%4s%4s%4s%4s%4s%4s %02x %02x %02x %02x %19s\n",
		addr6p[0], addr6p[1], addr6p[2], addr6p[3],
		addr6p[4], addr6p[5], addr6p[6], addr6p[7],
		 &if_index, &length_of_prefix, &scope_value, &if_flags, dev_name) != EOF) {

		if (strcmp(dev_name, "lo")) {
			sprintf(addr6, "%s:%s:%s:%s:%s:%s:%s:%s",
				addr6p[0], addr6p[1], addr6p[2], addr6p[3],
				addr6p[4], addr6p[5], addr6p[6], addr6p[7]);
				
			inet_pton(AF_INET6, addr6, &sap.sin6_addr);
			inet_ntop(AF_INET6, &sap.sin6_addr, addr6, sizeof(addr6));
			if (strncmp(addr6, "fe80::", strlen("fe80::")) == 0) {
				//fe80::开头为本机私有地址，算不算呢？
				//continue;
			}

			lockf(fileno(fp), F_ULOCK, 0);	
			fclose(fp);
			return 1;
		}
	}
	
	lockf(fileno(fp), F_ULOCK, 0);	
	fclose(fp);
	return 0;
}


int net_is_dhcp6c_work(const char *ifname)
{
	FILE *fp;
	char addr6[46] = {0}, dev_name[20] = {0};
	char addr6p[8][5];
	int length_of_prefix, scope_value, if_flags, if_index;
	struct sockaddr_in6 sap = {0};
	
	if (!ifname) return 0;
	
	fp = fopen(PROCNET_IFINET6_PATH, "r");
	if (!fp) return 0;

	lockf(fileno(fp), F_LOCK, 0);
	while (fscanf(fp, "%4s%4s%4s%4s%4s%4s%4s%4s %02x %02x %02x %02x %19s\n",
		addr6p[0], addr6p[1], addr6p[2], addr6p[3],
		addr6p[4], addr6p[5], addr6p[6], addr6p[7],
		 &if_index, &length_of_prefix, &scope_value, &if_flags, dev_name) != EOF) {

		if (!strcmp(dev_name, ifname)) {
			sprintf(addr6, "%s:%s:%s:%s:%s:%s:%s:%s",
				addr6p[0], addr6p[1], addr6p[2], addr6p[3],
				addr6p[4], addr6p[5], addr6p[6], addr6p[7]);
				
			inet_pton(AF_INET6, addr6, &sap.sin6_addr);
			inet_ntop(AF_INET6, &sap.sin6_addr, addr6, sizeof(addr6));
			if (strncmp(addr6, "fe80::", strlen("fe80::")) == 0) {
				//fe80::开头
				continue;
			}

			lockf(fileno(fp), F_ULOCK, 0);	
			fclose(fp);
			return 1;
		}
	}
	
	lockf(fileno(fp), F_ULOCK, 0);	
	fclose(fp);
	
	return 0;
}

int net_get_dhcp6c_pid(const char *ifname, int *pid)
{
	int ret = -1;
	FILE *fp;
	char path[128] = {0};
	char buf[128] = {0};
	
	if (!ifname || !pid) return ret;
	
	snprintf(path, sizeof(path), "/tmp/dhcp6c_%s.pid", ifname);
	fp = fopen(path, "r");
	if (!fp) return ret;
	
	if (fgets(buf, sizeof(buf), fp) != NULL) {
		*pid = atoi(buf);
		ret = 0;
	}
		
	fclose(fp);
	
	return ret;
}

int net_ipv4_trans_ipv6(const char *source, char *dest, int length)
{
	if (!source || !dest || length <= 0)
		return -1;
	
	if (strncmp(source, "::ffff:", strlen("::ffff:")) == 0 && strchr(source, '.')) {
		char ipv4[32] = {0};
		snprintf(ipv4, sizeof(ipv4), "%s", source+strlen("::ffff:"));
		
		struct in_addr s4 = {0};
		if (inet_pton(AF_INET, ipv4, (void*)&s4) != 1)
			return -1;
		
		char s6[4] = {0};
		s6[0] = s4.s_addr&0xff;
		s6[1] = s4.s_addr>>8&0xff;
		s6[2] = s4.s_addr>>16&0xff;
		s6[3] = s4.s_addr>>24&0xff;
		
		snprintf(dest, length, "::ffff:%x%.2x:%x%.2x", s6[0]&0xff, s6[1]&0xff, s6[2]&0xff, s6[3]&0xff);
		
		/*
		//struct in6_addr s6 = {0};
		memset(&s6.s6_addr, 0x0, sizeof(s6.s6_addr));
		s6.s6_addr[10] = 0xff;
		s6.s6_addr[11] = 0xff;
		s6.s6_addr[12] = s4.s_addr>>24&0xff;
		s6.s6_addr[13] = s4.s_addr>>16&0xff;
		s6.s6_addr[14] = s4.s_addr>>8&0xff;
		s6.s6_addr[15] = s4.s_addr&0xff;
		if (!inet_ntop(AF_INET6, &s6, dest, length))
			return -1;
		*/
		return 0;
	}
	
	
	return -1;
}

int net_get_dhcpv6gateway(const char *dhcpAddr, const char *dhcpPrefix, char *gateway, int length)
{
	if (!dhcpAddr || !dhcpPrefix || !gateway || length <= 0)
		return -1;
	
	int prefix;
	int i;
	struct sockaddr_in6 my_addr;
	char tmp[64] = {0};
	
	memset(&my_addr, 0x0, sizeof(my_addr));
	if (inet_pton(AF_INET6, dhcpAddr, &my_addr.sin6_addr) != 1)
		return -1;
	
	prefix = atoi(dhcpPrefix) / 8;
	

	for (i = prefix; i < 15; i++) {
		my_addr.sin6_addr.s6_addr[i] = 0;
	}
	my_addr.sin6_addr.s6_addr[i] = 1;
	
	if (!inet_ntop(AF_INET6,&my_addr.sin6_addr, tmp, sizeof(tmp)))
		return -1;
	
	snprintf(gateway, length, "%s", tmp);
	
	return 0;
}


//end


int net_get_ifaddr(const char *ifname, char *addr, int length)
{
	struct ifreq ifr;
	int skfd;
	struct sockaddr_in *saddr;
	if ( (skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		printf("socket error");
		return -1;
	}
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	if (ioctl(skfd, SIOCGIFADDR, &ifr) < 0)
	{
		close(skfd);
		return -1;
	}
	close(skfd);
	saddr = (struct sockaddr_in *) &ifr.ifr_addr;
	snprintf(addr, length, "%s", inet_ntoa(saddr->sin_addr));
	return 0;
}

int net_get_netmask(const char *ifname, char *addr, int length)
{
	struct ifreq ifr;
	int skfd;
	struct sockaddr_in *saddr;

	if ( (skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) 
	{
		printf("socket error");
		return -1;
	}

	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	if (ioctl(skfd, SIOCGIFNETMASK, &ifr) < 0) 
	{
		printf("net_get_netmask: ioctl SIOCGIFNETMASK");
		close(skfd);
		return -1;
	}
	close(skfd);

	saddr = (struct sockaddr_in *) &ifr.ifr_addr;
	snprintf(addr, length, "%s", inet_ntoa(saddr->sin_addr));
	return 0;
}

static int net_search_gateway(const char *ifname, char *buf, in_addr_t *gate_addr)
{
	char iface[16];
	unsigned long dest, gate;
	int iflags;

	sscanf(buf, "%15s\t%08lX\t%08lX\t%8X\t", iface, &dest, &gate, &iflags);
	if ((iflags & (RTF_UP | RTF_GATEWAY)) == (RTF_UP | RTF_GATEWAY) && !strcmp(ifname, iface))
	{
		*gate_addr = gate;
		return 0;
	}
	return -1;
}

int net_get_gateway(const char *ifname, char *addr, int length)
{
	struct in_addr inaddr;
	in_addr_t gate_addr;
	char buff[132];
	FILE *fp = fopen(PROCNET_ROUTE_PATH, "r");

	if (!fp)
	{
		printf("INET (IPv4) not configured in this system.");
		snprintf(addr, length, "0.0.0.0");
		return -1;
	}
	fgets(buff, 130, fp);
	while (fgets(buff, 130, fp) != NULL) 
	{
		if (net_search_gateway(ifname, buff, &gate_addr) == 0) 
		{
			fclose(fp);
			inaddr.s_addr = gate_addr;
			snprintf(addr, length, "%s", inet_ntoa(inaddr));	
			return 0;
		}
	}
	fclose(fp);
	snprintf(addr, length, "0.0.0.0");
	return -1;
}

int write_mac_conf(char *filename)
{
	if(filename[0] == '\0')
		return -1;
	char szMacAddr[32] = {0};
	unsigned char mac[8] = {0};

	struct ifreq tmp;
	int sock_mac;
	sock_mac = socket(AF_INET, SOCK_STREAM, 0);
	if( sock_mac == -1)
	{
	    return -1;
	}
	memset(&tmp,0,sizeof(tmp));
#if defined(_HI3798_)
	strncpy(tmp.ifr_name, DEVICE_NAME_ETH1, sizeof(tmp.ifr_name)-1);
#else
	strncpy(tmp.ifr_name, DEVICE_NAME_ETH0, sizeof(tmp.ifr_name)-1);
#endif

	if((ioctl(sock_mac, SIOCGIFHWADDR, &tmp)) < 0)
	{
	    close(sock_mac);
	    return -1;
	}
	memcpy(mac, tmp.ifr_hwaddr.sa_data, sizeof(unsigned char)*6);
	close(sock_mac);
	snprintf(szMacAddr, sizeof(szMacAddr), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	FILE* fp = fopen(filename, "w");
	if (!fp) 
		return -1;
	fwrite(szMacAddr, 1, strlen(szMacAddr), fp);
	fflush(fp);
	fclose(fp);
	return 0;
}

int read_mac_conf(char *mac)
{
	char buff[128] = {0};
	if(!mac){
		return -1;
	}
	if(!access(MS_MAC0_CONF, F_OK)){
		FILE *fp = fopen(MS_MAC0_CONF, "r");
		if (!fp) 
			return -1;
		fread(buff, 1, sizeof(buff), fp);
		fclose(fp);
	}
	if(buff[0] != '\0');
		snprintf(mac, 64, "%s", buff);
	//printf("[david debug] read_mac_conf mac:%s\n", mac);
	return 0;
}

#if 0
int net_search_gateway_ex(char *buf, in_addr_t *gate_addr)
{
	char iface[16];
	unsigned long dest, gate;
	int iflags;

	sscanf(buf, "%s\t%08lX\t%08lX\t%8X\t", iface, &dest, &gate, &iflags);
	if ((iflags & (RTF_UP | RTF_GATEWAY)) == (RTF_UP | RTF_GATEWAY))
	{
		*gate_addr = gate;
		return 0;
	}
	return -1;
}

int net_add_gateway(in_addr_t addr)
{
	struct rtentry rt;
	int skfd;
	struct sockaddr_in sa = {
		sin_family:	PF_INET,
		sin_port:	0
	};

	/* Clean out the RTREQ structure. */
	memset((char *) &rt, 0, sizeof(struct rtentry));

	/* Fill in the other fields. */
	rt.rt_flags = (RTF_UP | RTF_GATEWAY);

	rt.rt_dst.sa_family = PF_INET;
	rt.rt_genmask.sa_family = PF_INET;

	sa.sin_addr.s_addr = addr;
	memcpy((char *) &rt.rt_gateway, (char *) &sa, sizeof(struct sockaddr));

	/* Create a socket to the INET kernel. */
	if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		msdebug(DEBUG_ERR, "socket error");
		return -1;
	}
	/* Tell the kernel to accept this route. */
	if (ioctl(skfd, SIOCADDRT, &rt) < 0) 
	{
		msdebug(DEBUG_ERR, "net_add_gateway: ioctl SIOCADDRT");
		close(skfd);
		return -1;
	}
	/* Close the socket. */
	close(skfd);
	return (0);
}

int net_del_gateway(in_addr_t addr)
{
	struct rtentry rt;
	int skfd;
	struct sockaddr_in sa = {
		sin_family:	PF_INET,
		sin_port:	0
	};

	/* Clean out the RTREQ structure. */
	memset((char *) &rt, 0, sizeof(struct rtentry));

	/* Fill in the other fields. */
	rt.rt_flags = (RTF_UP | RTF_GATEWAY);

	rt.rt_dst.sa_family = PF_INET;
	rt.rt_genmask.sa_family = PF_INET;

	sa.sin_addr.s_addr = addr;
	memcpy((char *) &rt.rt_gateway, (char *) &sa, sizeof(struct sockaddr));

	/* Create a socket to the INET kernel. */
	if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		msdebug(DEBUG_ERR, "socket error");
		return -1;
	}
	/* Tell the kernel to accept this route. */
	if (ioctl(skfd, SIOCDELRT, &rt) < 0)
	{
		msdebug(DEBUG_ERR, "net_del_gateway: ioctl SIOCDELRT");
		close(skfd);
	return -1;
	}
	/* Close the socket. */
	close(skfd);
	return (0);
}

int net_set_gateway(const char *ifname, const char *addr)
{
	in_addr_t gate_addr;
	char buff[132];
	struct sockaddr_in sa = {
		sin_family:	PF_INET,
		sin_port:	0
	};	
	FILE *fp = fopen(PROCNET_ROUTE_PATH, "r");

	if (!fp) 
	{
		msdebug(DEBUG_ERR, PROCNET_ROUTE_PATH);
		msprintf("INET (IPv4) not configured in this system.");
		return -1;
	}
	fgets(buff, 130, fp);
	while (fgets(buff, 130, fp) != NULL) 
	{
		if (net_search_gateway(ifname, buff, &gate_addr) == 0) 
		{
			net_del_gateway(gate_addr);
		}
	}
	fclose(fp);
	inet_aton(addr, &sa.sin_addr);
	return net_add_gateway(sa.sin_addr.s_addr);
}

int net_set_gateway_ex(const char *addr)
{
	in_addr_t gate_addr;
	char buff[132];
	struct sockaddr_in sa = {
		sin_family:	PF_INET,
		sin_port:	0
	};	
	FILE *fp = fopen(PROCNET_ROUTE_PATH, "r");
	if (!fp) 
	{
		msdebug(DEBUG_ERR, PROCNET_ROUTE_PATH);
		msprintf("INET (IPv4) not configured in this system.");
		return -1;
	}
	fgets(buff, 130, fp);
	while (fgets(buff, 130, fp) != NULL) 
	{
		if (net_search_gateway_ex(buff, &gate_addr) == 0) 
		{
			net_del_gateway(gate_addr);
		}
	}
	fclose(fp);
	inet_aton(addr, &sa.sin_addr);
	return net_add_gateway(sa.sin_addr.s_addr);
}
#endif
