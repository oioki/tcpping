/***************************************************************************************************
  TCP Ping
  Sequentially creates tcp connections to the specified host and measures the latency.
  Author: Alexander Tarasov aka oioki
***************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <errno.h>
#include <sys/time.h>
#include <math.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <limits.h>


// I use it mostly for remote servers
#define DEFAULT_PORT 22


// return time period between t1 and t2 (in milliseconds)
long int timeval_subtract(struct timeval *t2, struct timeval *t1)
{
    return (t2->tv_usec + 1000000 * t2->tv_sec) - (t1->tv_usec + 1000000 * t1->tv_sec);
}

// sequence number
static int seq = 0;
static int cnt_successful = 0;

// aggregate stats
unsigned long int diffMin = ULONG_MAX;
unsigned long int diffAvg;
unsigned long int diffMax = 0;
unsigned long int diffSum = 0;
unsigned long int diffSum2 = 0;
unsigned long int diffMdev;

// address
struct sockaddr_in addrServer;

int running = 1;


// one ping
int ping(char * ip, int port)
{
    seq++;

    // creating new socket for each new ping
    int sfdInet = socket(PF_INET, SOCK_STREAM, 0);
    if ( sfdInet == -1 )
    {
        fprintf(stderr, "Failed to create INET socket, error %d\n", errno);
        return 1;
    }

    // adjusting connection timeout = 1 second
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    int err = setsockopt (sfdInet, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
    if ( err < 0 )
        fprintf(stderr, "Failed setsockopt SO_RCVTIMEO, error %d\n", errno);
    err = setsockopt (sfdInet, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));
    if ( err < 0 )
        fprintf(stderr, "Failed setsockopt SO_SNDTIMEO, error %d\n", errno);

    // note the starting time
    struct timeval tvBegin, tvEnd, tvDiff;
    gettimeofday(&tvBegin, NULL);

    // try to make connection
    err = connect(sfdInet, (struct sockaddr *) &addrServer, sizeof(struct sockaddr_in));
    if ( err == -1 )
    {
        switch ( errno )
        {
            case EMFILE:
                fprintf(stderr, "Error (%d): too many files opened", errno);
                break;
            case ECONNREFUSED:
                fprintf(stderr, "almost Connection refused (error %d) %s:%d, seq=%d\n", errno, ip, port, seq);
                err = close(sfdInet);
                break;
            case EHOSTUNREACH:
                fprintf(stderr, " ....  Host unreachable (error %d) while connecting %s:%d, seq=%d\n", errno, ip, port, seq);
                err = close(sfdInet);
                break;
            case EINPROGRESS:
                fprintf(stderr, " ....  Timeout (error %d) while connecting %s:%d, seq=%d\n", errno, ip, port, seq);
                err = close(sfdInet);
                break;
            default:
                fprintf(stderr, "Error (%d) while connecting %s:%d, seq=%d\n", errno, ip, port, seq);
        }

        // sleeping 1 sec until the next ping
        sleep(1);

        return 1;
    }

    // note the ending time and calculate the duration of TCP ping
    gettimeofday(&tvEnd, NULL);
    long int diff = timeval_subtract(&tvEnd, &tvBegin);
    int secs = diff / 1000000;
    printf("  OK   Connected to %s:%d, seq=%d, time=%0.3lf ms\n", ip, port, seq, diff/1000.);
    cnt_successful++;

    // changing aggregate stats
    if ( diff < diffMin ) diffMin = diff;
    if ( diff > diffMax ) diffMax = diff;
    diffSum  += diff;
    diffSum2 += diff*diff;

    // OK, closing the connection
    err = close(sfdInet);

    // sleeping until the beginning of the next second
    struct timespec ts;
    ts.tv_sec  = 0;
    ts.tv_nsec = 1000 * ( 1000000*(1+secs) - diff );
    nanosleep(&ts, &ts);

    return 0;
}

void intHandler()
{
    running = 0;
}

int main(int argc, char * argv[])
{
    if ( argc == 1 )
    {
        printf("Usage: %s hostname [port]\n", argv[0]);
        return 1;
    }
    char * host = argv[1];
    int port = (argc==2) ? DEFAULT_PORT : atoi(argv[2]);

    // resolving the hostname
    struct hostent * he;
    extern h_errno;
    he = gethostbyname(host);
    if ( he == NULL )
    {
        fprintf(stderr, "tcpping: unknown host %s (error %d)\n", host, h_errno);
        return 1;
    }

    // filling up `sockaddr_in` structure
    memset(&addrServer, 0, sizeof(struct sockaddr_in));
    addrServer.sin_family = AF_INET;
    memcpy(&(addrServer.sin_addr.s_addr), he->h_addr, he->h_length);
    addrServer.sin_port = htons(port);

    // first IP address as the target
    struct in_addr ** addr_list = (struct in_addr **) he->h_addr_list;
    char ip[16];
    strcpy(ip, inet_ntoa(*addr_list[0]));

    // Ctrl+C handler
    signal(SIGINT, intHandler);

    // note the starting time
    struct timeval tvBegin, tvEnd, tvDiff;
    gettimeofday(&tvBegin, NULL);


    // main loop
    while (running) ping(ip, port);


    // note the ending time and calculate the duration of TCP ping
    gettimeofday(&tvEnd, NULL);
    long int diff = timeval_subtract(&tvEnd, &tvBegin);

    // summary
    printf ("\n--- %s tcpping statistics ---\n", host);
    printf ("%d packets transmitted, %d received, %d%% packet loss, time %ldms\n", seq, cnt_successful, 100-100*cnt_successful/seq, diff/1000);
    if ( cnt_successful > 0 )
    {
        diffAvg  = diffSum/cnt_successful;
        diffMdev = sqrt( diffSum2/cnt_successful - diffAvg*diffAvg );
        printf ("rtt min/avg/max/mdev = %0.3lf/%0.3lf/%0.3lf/%0.3lf ms\n", diffMin/1000.,diffAvg/1000.,diffMax/1000.,diffMdev/1000.);
    }

    return 0;
}
