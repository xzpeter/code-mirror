#ifndef  __DBG_H__
#define  __DBG_H__

/* settings of debug level:
1: vital and most high level messages ( e.g. errors )
2: vital and low level messages ( e.g. low level errors )
3: high level normal messages
4: low level logging messages (verbose) */

#define  LOG_FILE  "log.txt"

extern int debug_level;

#define  dbg(level, format, arg...) do { FILE *fd = fopen(LOG_FILE, "a");if(fd==NULL) {puts("failed log.");break;} if(level<=debug_level){/*fprintf(stderr, "%s:%d: " format "\n", __func__,__LINE__, ##arg);*/fprintf(fd, "%s:%d: " format "\n", __func__,__LINE__, ##arg);} fclose(fd);} while(0)

#endif
