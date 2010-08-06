#ifndef  __DBG_H__
#define  __DBG_H__

#ifndef  DEBUG_LEVEL
#define  DEBUG_LEVEL 1
#endif

// #define  dbg(level, format, arg...)  do {if(level<=DEBUG_LEVEL){fprintf(stderr, "%s()[%s:%d]: " format "\n", __func__,__FILE__,__LINE__, ##arg);}} while(0)
#define  dbg(level, format, arg...)  do {if(level<=DEBUG_LEVEL){fprintf(stderr, "%s:%d: " format "\n", __FILE__,__LINE__, ##arg);}} while(0)

#endif
