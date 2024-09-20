#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/prctl.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>

#include <sys/time.h>

#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <resolv.h>  
#include <sys/syscall.h>
#include "ntp_client.h" 

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif



#define PST_TIMEZONE "PST8PDT,M3.2.0,M11.1.0|1.1361"
#define BEIJING_TIMEZONE "CST-8|1.1297"

#define NTP_TIMESTAMP_DELTA 2208988800ull

#define LI(packet)   (uint8_t) ((packet.li_vn_mode & 0xC0) >> 6) // (li   & 11 000 000) >> 6
#define VN(packet)   (uint8_t) ((packet.li_vn_mode & 0x38) >> 3) // (vn   & 00 111 000) >> 3
#define MODE(packet) (uint8_t) ((packet.li_vn_mode & 0x07) >> 0) // (mode & 00 000 111) >> 0
#define USEC(x) ( ( (x) >> 12 ) - 759 * ( ( ( (x) >> 10 ) + 32768 ) >> 16 ) )
#define HUB_TIME_FILE          "/data/hub_time.dat"

int hostname_to_ipv4(const char* hostname, char * pIPBuf, int iIPlen)
{
    struct addrinfo hints;
    struct addrinfo * pAddrinfo = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; /* AF_UNSPEC AF_INET6 or AF_INET */
    hints.ai_socktype = SOCK_DGRAM;

    int errcode  = getaddrinfo(hostname, NULL, &hints, &pAddrinfo);
    if (errcode != 0)
    {
        printf("getaddrinfo: %s\n", gai_strerror(errcode));
        return -1;
    }

    if (pAddrinfo)
    {
        if(pAddrinfo->ai_family == AF_INET)
        {
            if( NULL == inet_ntop(AF_INET,
                         &((struct sockaddr_in *)pAddrinfo->ai_addr)->sin_addr,
                         pIPBuf, iIPlen))
            {
                printf("inet_ntop\r\n");
                return -1;
            }
            printf("%s\r\n", pIPBuf);
        }

        freeaddrinfo(pAddrinfo); // free the linked list
    }
    return 0;
}

int setup_receive(int usd, unsigned int interface, short port)
{
    struct sockaddr_in sa_rcvr;
    const int opt = 1;
    memset(&sa_rcvr, 0, sizeof sa_rcvr);
    sa_rcvr.sin_family=AF_INET;
    sa_rcvr.sin_addr.s_addr=htonl(interface);
    sa_rcvr.sin_port=htons(port);

    if(setsockopt(usd, SOL_SOCKET, SO_REUSEADDR, (const void*)&opt, sizeof(opt)) == -1)
    {
        printf("setsockopt error.\r\n");  //失败了使用默认设置
    }

    struct linger lin;
    lin.l_onoff = 1;                    //关闭连接后，端口立即关闭
    lin.l_linger = 0;                   //关闭连接后，端口立即关闭
    if(setsockopt(usd, SOL_SOCKET, SO_LINGER, (const void*)&lin, sizeof(lin)) < 0)          //关掉socker后立即释放端口
    {
        printf("setsockopt SO_LINGER error.\r\n");  //失败了使用默认设置
    }

    struct timeval timeout;
    timeout.tv_sec = 10;  // recvfrom()超时
    timeout.tv_usec = 0;
    if (setsockopt(usd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(struct timeval)) < 0)
    {
        printf("setsockopt SO_RCVTIMEO error\r\n");  //失败了使用默认设置
    }

    timeout.tv_sec = 10;  // send()超时
    timeout.tv_usec = 0;
    if (setsockopt(usd, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(struct timeval)) < 0)
    {
        printf("setsockopt SO_SNDTIMEO error\r\n");  //失败了使用默认设置
    }

    if(bind(usd,(struct sockaddr *) &sa_rcvr,sizeof sa_rcvr) == -1)
    {
        //perror("bind");
        printf("bind error,errno[%d]======>[%s]\r\n",errno,strerror(errno));
        fprintf(stderr,"could not bind to udp port %d\r\n",port);
        //exit(1);
        return -1;
    }

    //printf("bind ok.");
    return 0;
    /* listen(usd,3); this isn't TCP; thanks Alexander! */
}

int my_hostbyname(char *hostname,char *ip)
{
    char str[32];
    struct hostent *hptr;
    char **pptr;
    int i = 0;
    for(i = 0; i < 10 ; i++)
    {
        if((hptr=gethostbyname(hostname))==NULL)
        {
            printf("gethostbyname error for host: %s\r\n",hostname);
            usleep(100*1000);
            continue;
        }
        switch(hptr->h_addrtype)//判断socket类型
        {
            case AF_INET:  //IP类为AF_INET
                pptr=hptr->h_addr_list; //IP地址数组
                for(;*pptr!=NULL;pptr++)
                {
                    printf("\taddress: %s\r\n",inet_ntop(hptr->h_addrtype,*pptr,str,sizeof(str)));
                    strcpy(ip,inet_ntop(hptr->h_addrtype,*pptr,str,sizeof(str)));
                    return 0; 
                }
                break;
            default:
                printf("unknown address type\r\n");
                break;
        }
    }
    return 0;
}

int time_calibration(char* host_name)
{
    int sockfd, n; // Socket file descriptor and the n return result from writing/reading from the socket.

    int portno = 123; // NTP UDP port number.

    int ret = -1;

    struct timeval tv_set;

    //char* host_name = "us.pool.ntp.org"; // NTP server host-name.
    if (host_name == NULL)
    {
        printf("host name null.");
        return -1;
    }

    // Structure that defines the 48 byte NTP packet protocol.

    //printf("ntp start hostname %s", host_name);

    typedef struct
    {

        uint8_t li_vn_mode;      // Eight bits. li, vn, and mode.
        // li.   Two bits.   Leap indicator.
        // vn.   Three bits. Version number of the protocol.
        // mode. Three bits. Client will pick mode 3 for client.

        uint8_t stratum;         // Eight bits. Stratum level of the local clock.
        uint8_t poll;            // Eight bits. Maximum interval between successive messages.
        uint8_t precision;       // Eight bits. Precision of the local clock.

        uint32_t rootDelay;      // 32 bits. Total round trip delay time.
        uint32_t rootDispersion; // 32 bits. Max error aloud from primary clock source.
        uint32_t refId;          // 32 bits. Reference clock identifier.

        uint32_t refTm_s;        // 32 bits. Reference time-stamp seconds.
        uint32_t refTm_f;        // 32 bits. Reference time-stamp fraction of a second.

        uint32_t origTm_s;       // 32 bits. Originate time-stamp seconds.
        uint32_t origTm_f;       // 32 bits. Originate time-stamp fraction of a second.

        uint32_t rxTm_s;         // 32 bits. Received time-stamp seconds.
        uint32_t rxTm_f;         // 32 bits. Received time-stamp fraction of a second.

        uint32_t txTm_s;         // 32 bits and the most important field the client cares about. Transmit time-stamp seconds.
        uint32_t txTm_f;         // 32 bits. Transmit time-stamp fraction of a second.

    } ntp_packet;              // Total: 384 bits or 48 bytes.

    // Create and zero out the packet. All 48 bytes worth.

    ntp_packet packet = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    memset( &packet, 0, sizeof( ntp_packet ) );

    // Set the first byte's bits to 00,011,011 for li = 0, vn = 3, and mode = 3. The rest will be left set to zero.

    *( ( char * ) &packet + 0 ) = 0x1b; // Represents 27 in base 10 or 00011011 in base 2.

    // Create a UDP socket, convert the host-name to an IP address, set the port number,
    // connect to the server, send the packet, and then read in the return packet.

    struct sockaddr_in serv_addr; // Server address data structure.
    sockfd = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP ); // Create a UDP socket.

    if ( sockfd < 0 )
    {
        printf("ERROR opening socket %d\r\n", sockfd);
        return -1;
    }

    ret = setup_receive(sockfd, INADDR_ANY, 0);
    if ( ret < 0 )
    {
        printf("setup receive error, ret=%d\r\n", ret);
        close(sockfd);
        return -1;
    }

    // Zero out the server address structure.
    bzero( ( char* ) &serv_addr, sizeof( serv_addr ) );
    serv_addr.sin_family = AF_INET;
    char tmpIP[16] = {0};
    ret = hostname_to_ipv4(host_name, tmpIP, 16);
    if(ret >= 0 )
    {
        serv_addr.sin_addr.s_addr = inet_addr(tmpIP);
        printf("=================pDevParm->ntp_ip:%s======host_name:%s===========\r\n", tmpIP,host_name);
    }
    else
    {
        if (0 != res_init())
        {
            return -1;
        }
        char ipbuff[32];
        memset(ipbuff,0,32);
        my_hostbyname(host_name,ipbuff); // Convert URL to IP.
        if (strlen(ipbuff) > 0)
        {
            serv_addr.sin_addr.s_addr = inet_addr(ipbuff);
            printf("=================ipbuff:%s=================\r\n", ipbuff);
            return -1;
        }
    }

    // Convert the port number integer to network big-endian style and save it to the server address structure.
    serv_addr.sin_port = htons( portno);

    // Call up the server using its IP address and port number.
    if ( connect( sockfd, ( struct sockaddr * ) &serv_addr, sizeof( serv_addr) ) < 0 )
    {
        printf("ERROR connecting");
        close(sockfd);
        return -1;
    }

    // Send it the NTP packet it wants. If n == -1, it failed.
    n = write( sockfd, ( char*) &packet, sizeof( ntp_packet ) );
    if ( n < 0 )
    {
        printf("ERROR writing to socket, ret=%d.\r\n", n);
        close(sockfd);
        return -1;
    }

    // Wait and receive the packet back from the server. If n == -1, it failed.
    n = read( sockfd, ( char* ) &packet, sizeof( ntp_packet ) );

    if ( n < 0 )
    {
        printf("ERROR reading from socket, ret=%d\r\n", n);
        close(sockfd);
        return -1;
    }

    // These two fields contain the time-stamp seconds as the packet left the NTP server.
    // The number of seconds correspond to the seconds passed since 1900.
    // ntohl() converts the bit/byte order from the network's to host's "endianness".

    packet.txTm_s = ntohl( packet.txTm_s ); // Time-stamp seconds.
    packet.txTm_f = ntohl( packet.txTm_f ); // Time-stamp fraction of a second.

    // Extract the 32 bits that represent the time-stamp seconds (since NTP epoch) from when the packet left the server.
    // Subtract 70 years worth of seconds from the seconds since 1900.
    // This leaves the seconds since the UNIX epoch of 1970.
    // (1900)------------------(1970)**************************************(Time Packet Left the Server)

    time_t txTm = ( time_t ) ( packet.txTm_s - NTP_TIMESTAMP_DELTA );

    // Print the time we got from the server, accounting for local timezone and conversion from UTC time.

    printf("Time: %s\r\n", ctime( ( const time_t* ) &txTm ));

    tv_set.tv_sec  = txTm;
    tv_set.tv_usec = USEC(packet.txTm_f);
    ret = settimeofday(&tv_set, NULL);
    if (ret < 0 )
    {
        printf("set time failed, ret=%d\r\n", ret);
        close(sockfd);
        return -1;
    }

    close(sockfd);

    return 0;
}


int set_timetone(char *time_tone)
{
    char buf[128] = {0};
    char tmp[128] = {0};
    FILE * fp = NULL;
    if ( time_tone == NULL )
    {
        printf("====>>>> timezone empty\r\n");
        return -1;
    }

    printf("input timetone:%s\r\n", time_tone);
    if (strlen(time_tone) >= 128)
    {
        printf("====>>> time_tone out of len\r\n");
        return -1;
    }

    strncpy(tmp, time_tone, 128-1);

    char *p = strchr(tmp, '|'); // 去掉APP传下来的 “ | 索引号”  如 | 65
    if (p)
    {
        *p = 0; // 截断APP的索引号
    }
    p = NULL;

    if (strncmp(tmp,"GMT", 3) == 0)
    {
        if (tmp[3] == '+')  // TZ的值同APP相反
        {
            tmp[3] = '-';
        }
        else if (tmp[3] == '-')
        {
            tmp[3] = '+';
        }
    }

    printf("======tmp:%s==============\n",tmp);
    setenv("TZ",tmp,1);

    tzset(); // 使用环境变量TZ的当前设置把值赋给三个全局变量:daylight,timezone和tzname
    fp = fopen("/etc/TZ", "wb");
    if (fp != NULL)
    {
        int ilen = strlen(tmp);
        tmp[ilen]   = '\n';     // 要加换行符系统才生效;
        tmp[ilen+1] = '\0';
        fwrite(tmp, 1, ilen+1, fp);
        fclose(fp);
    }
    else
    {
        //SYS_ERROR("fopen /etc/TZ error");
    }

    return 0;
}

int System_SaveTime(void)
{
    int fd = open(HUB_TIME_FILE, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0644);
    if(fd == -1)
    {
        printf("open file %s failed\r\n", HUB_TIME_FILE);
    }
    else
    {
        time_t cur_time = time(NULL);
        write(fd, (char *)&cur_time, sizeof(time_t));
        fsync(fd);
        close(fd);
        printf("save system time:%ld\r\n", cur_time);
    }
    return 0;
}

static void *time_ntp_thread(void *args)
{
    int ret = 0;
    int i = 0, server_num = 0;
    //char cmd_str[64] = {0};
    char ipaddr_str[32];
    char *ntp_server_list[] =
    {
        //"1.cn.pool.ntp.org",
        "time.nist.gov",            //美国标准技术院
        //"ntp1.aliyun.com",            //阿里云服务器
        "pool.ntp.org",                 //NTP时间服务器
        "us.pool.ntp.org",              //NTP时间服务器
        "north-america.pool.ntp.org",   //NTP时间服务器
        "europe.pool.ntp.org",          //NTP时间服务器
        //"ntp2.aliyun.com",            //阿里云服务器
        //"ntp.ntsc.ac.cn",         //中科院国家授时中心时间 www.time.ac.cn
        //"time.pool.aliyun.com",       //阿里云服务器
        //"time1.cloud.tencent.com",    //腾讯云时间
        //"ntp3.aliyun.com",            //阿里云服务器
        //"2.cn.pool.ntp.org",
        "time4.google.com",
        "time.apple.com",
        "time.windows.com",
        "0.android.pool.ntp.org",       //NTP时间服务器
    };
    prctl(PR_SET_NAME,"ntp");

    printf("start ntp thread %lu.\r\n", syscall(__NR_gettid));
#if 0  //设置时区
    HUB_BASE_INFO *pBS = NV_GetBSConfig();
    printf("===time_ntp_thread====pBS->time_tone:%s======\n",pBS->time_tone);
    ret = zx_set_timetone(pBS->time_tone);
    if (ret < 0)
    {
        printf("set timezone failed ret=%d.", ret);
    }
#endif
    
    set_timetone(BEIJING_TIMEZONE);

    //update_timezone();
    server_num = sizeof(ntp_server_list)/sizeof(ntp_server_list[0]);
    while (1)
    {
        for (i = 0; i < server_num; i++)
        {
            ret = time_calibration(ntp_server_list[i]);
            if (ret == 0)
            {
                printf("ntp with server %d %s success.\r\n", i, ntp_server_list[i]);
                System_SaveTime();
                goto ntp_thread_exit;
            }
            sleep(2);
        }
        sleep(5);
    }
ntp_thread_exit:
    printf("ntp thread exit.\r\n");
    pthread_exit(0);
}



void NtpClient_Start(void)
{
    // ThreadParams threadp;
    // memset(&threadp,0,sizeof(ThreadParams));
    // strcpy(threadp.thread_name,"time_ntp_thread");
    // threadp.thread_func = time_ntp_thread;
    // int ret = CommLib_threadCreate(&threadp);
    // if(ret != 0)
    // {
    //     printf("creat Record LocalWrite Thread fail \r\r\r\n");
    //     return ;
    // }


    pthread_t keyThread;
	int s32Ret = pthread_create(&keyThread, NULL, time_ntp_thread, NULL);

    pthread_detach(keyThread);
}




#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
