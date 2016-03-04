#ifdef __cplusplus
extern "C"
{
#endif

#ifndef mdnsd_h
#define mdnsd_h
#include "1035.h"

#ifdef WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS   /* allow the old inet_addr() call at lien 521 of mdnsd.c */

// the next is supposed to prevent winsock.h being included in <windows.h>
#define _WINSOCKAPI_
// this is supposed to do the same job
#define WIN32_LEAN_AND_MEAN

#ifdef WIN32
#include <time.h>
#else
#include <sys/time.h>
#include <arpa/inet.h>
#endif

#include <winsock2.h> // this puts the timeval struct within scope, to avoid c4115 warning
struct timezone {
	int tz_minuteswest;     /* minutes west of Greenwich */
	int tz_dsttime;         /* type of DST correction */
}; // ditto, need a definition which is in scope - winsock2.h does not have it
//Do our own implementation, from code on the web
int gettimeofday(struct timeval * tp, struct timezone * tzp);
#endif

typedef struct mdnsd_struct *mdnsd; // main daemon data
typedef struct mdnsdr_struct *mdnsdr; // record entry
// answer data
typedef struct mdnsda_struct
{
    unsigned char *name;
    unsigned short int type;
    unsigned long int ttl;
    unsigned short int rdlen;
    unsigned char *rdata;
    unsigned long int ip; // A
    unsigned char *rdname; // NS/CNAME/PTR/SRV
    struct { unsigned short int priority, weight, port; } srv; // SRV
} *mdnsda;

struct query
{
    char *name;
    int type;
    unsigned long int nexttry;
    int tries;
    int (*answer)(mdnsda, void *);
    void *arg;
    struct query *next, *list;
};

struct cached
{
    struct mdnsda_struct rr;
    struct query *q;
    struct cached *next;
};

///////////
// Global functions
//
// create a new mdns daemon for the given class of names (usually 1) and maximum frame size
mdnsd mdnsd_new(int rr_class, int frame);
//
// gracefully shutdown the daemon, use mdnsd_out() to get the last packets
void mdnsd_shutdown(mdnsd d);
//
// flush all cached records (network/interface changed)
//void mdnsd_flush(mdnsd d);  BEW removed, this function is never called
//
// free given mdnsd (should have used mdnsd_shutdown() first!)
void mdnsd_free(mdnsd d);

///////////

///////////
// I/O functions
//
// incoming message from host (to be cached/processed)
void mdnsd_in(mdnsd d, struct message *m, unsigned long int ip, unsigned short int port);
//
// outgoing messge to be delivered to host, returns >0 if one was returned and m/ip/port set
int mdnsd_out(mdnsd d, struct message *m, unsigned long int *ip, unsigned short int *port);
//
// returns the max wait-time until mdnsd_out() needs to be called again
struct timeval *mdnsd_sleep(mdnsd d);
//
////////////

///////////
// Q/A functions
//
// register a new query
//   answer(record, arg) is called whenever one is found/changes/expires (immediate or anytime after, mdnsda valid until ->ttl==0)
//   either answer returns -1, or another mdnsd_query with a NULL answer will remove/unregister this query
void mdnsd_query(mdnsd d, char *host, int type, int (*answer)(mdnsda a, void *arg), void *arg);
//
// returns the first (if last == NULL) or next answer after last from the cache
//   mdnsda only valid until an I/O function is called
mdnsda mdnsd_list(mdnsd d, char *host, int type, mdnsda last);
//
///////////

///////////
// Publishing functions
//
// create a new unique record (try mdnsda_list first to make sure it's not used)
//   conflict(arg) called at any point when one is detected and unable to recover
//   after the first data is set_*(), any future changes effectively expire the old one and attempt to create a new unique record
mdnsdr mdnsd_unique(mdnsd d, char *host, int type, long int ttl, void (*conflict)(char *host, int type, void *arg), void *arg);
//
// create a new shared record
mdnsdr mdnsd_shared(mdnsd d, char *host, int type, long int ttl);
//
// de-list the given record
void mdnsd_done(mdnsd d, mdnsdr r);
//
// these all set/update the data for the given record, nothing is published until they are called
void mdnsd_set_raw(mdnsd d, mdnsdr r, char *data, int len);
void mdnsd_set_host(mdnsd d, mdnsdr r, char *name);
void mdnsd_set_ip(mdnsd d, mdnsdr r, unsigned long int ip);
void mdnsd_set_srv(mdnsd d, mdnsdr r, int priority, int weight, int port, char *name);
void _gc(mdnsd d);

// BEW 2Dec15 added these cache deletion functions
void my_c_expire(struct cached **list);
void my_gc(mdnsd d); // roll my own

///////////


#endif


#ifdef __cplusplus
}
#endif
