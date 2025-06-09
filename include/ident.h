/**
 * @file ident.h
 * @brief Minimal ident header used for testing and documentation.
 */

#ifndef IDENT_H
#define IDENT_H

#include <sys/types.h>
#include <netinet/in.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IDBUFSIZE 2048
#define IDPORT    113

/** Descriptor for an ident session. */
typedef struct {
    int   fd;             /**< Socket descriptor. */
    char  buf[IDBUFSIZE]; /**< Internal buffer. */
} ident_t;

/** Result of a completed ident query. */
typedef struct {
    int    lport;      /**< Local port number. */
    int    fport;      /**< Remote port number. */
    char*  identifier; /**< Typically the user name. */
    char*  opsys;      /**< Operating system. */
    char*  charset;    /**< Character set used. */
} IDENT;

/** Retrieve the file descriptor from an ident session. */
#define id_fileno(ID) ((ID)->fd)

ident_t* id_open(struct in_addr* laddr, struct in_addr* faddr,
                 struct timeval* timeout);
int      id_close(ident_t* id);
int      id_query(ident_t* id, int lport, int fport,
                  struct timeval* timeout);
int      id_parse(ident_t* id, struct timeval* timeout, int* lport,
                  int* fport, char** identifier, char** opsys,
                  char** charset);
IDENT*   ident_lookup(int fd, int timeout);
char*    ident_id(int fd, int timeout);
IDENT*   ident_query(struct in_addr* laddr, struct in_addr* raddr,
                     int lport, int rport, int timeout);
void     ident_free(IDENT* id);
extern char id_version[];
char*    id_strdup(const char* str);
char*    id_strtok(char* cp, char* cs, char* dc);

#ifdef __cplusplus
}
#endif

#endif /* IDENT_H */
