/*
  file: ring.c
  desc: a simpler ring buffer
  mail: xzpeter@gmail.com

  something about the MACROS used:

  1. DEBUG_RING
  we should define DEBUG_RING to debug the buffer
*/

/* since we don't support debugging the code under kernel mode, we should
   detect it if there is any attempt. */
#ifdef __KERNEL__
  #include <linux/slab.h>
  #include <linux/string.h>
  #define _rb_malloc(n) kmalloc(n, GFP_KERNEL)
  #define _rb_free kfree
  #define _rb_print printk
#else /* ifdef USED_IN_KERNEL */
  #include <stdio.h>
  #include <stdlib.h>
  #include <time.h>
  #include <string.h>
  #define _rb_malloc malloc
  #define _rb_free free
  #define _rb_print printf
#endif /* ifdef USED_IN_KERNEL */

#ifdef DEBUG_RING_BUFFER
  #define dbg(fmt, args...) _rb_print("[rb] " fmt "\n", ##args)
#else /* ifdef DEBUG_RING_BUFFER */
  #define dbg(fmt, args...)
#endif /* ifdef DEBUG_RING_BUFFER */

struct ring_buffer {
    char *buf;                  /* space to store data */
    unsigned int total;         /* total size of the buffer. should be 1
                                   smaller than size of BUF */
    unsigned int used;          /* how many space used in the buffer */
    char *read;                 /* the read/write pointers */
    char *write;
};

#define  RB_MIN(a, b)  ((a>b)?(b):(a))

/* this will create a ring buffer with size=SIZE and return the struct pointer */
struct ring_buffer *ring_buffer_create(unsigned int size)
{
    struct ring_buffer *ring;
    if (size - 1 <= 1)
        return NULL;
    ring = _rb_malloc(sizeof(struct ring_buffer));
    if (!ring) {
        dbg("failed malloc ring");
        return NULL;
    }
    ring->buf = _rb_malloc(size);
    if (!ring->buf) {
        dbg("failed malloc buf");
        return NULL;
    }
    ring->total = size - 1;
    ring->used = 0;
    ring->read = ring->write = ring->buf;
    return ring;
}

unsigned int ring_buffer_total(struct ring_buffer *ring)
{
    return ring->total;
}

unsigned int ring_buffer_used(struct ring_buffer *ring)
{
    return ring->used;
}

unsigned int ring_buffer_rest(struct ring_buffer *ring)
{
    return (ring_buffer_total(ring) - ring_buffer_used(ring));
}

/* write data BUFFE with len LEN to ring buffer RING.
   ret >= 0 if written, and value should be the length of written (can be
   smaller than LEN). ret < 0 if failed */
unsigned int ring_buffer_write(struct ring_buffer *ring, char *buffer,
                               unsigned int len)
{
    unsigned int to_write, rest = ring->total - ring->used;
    if (!buffer)
        return -1;
    len = RB_MIN(len, rest);
    to_write = (ring->total + 1) - (ring->write - ring->buf);
    to_write = RB_MIN(to_write, len);

    /* the first write */
    memcpy(ring->write, buffer, to_write);
    buffer += to_write;

    to_write = len - to_write;
    if (0 == to_write) {
        ring->write += len;
        if (ring->write > ring->buf+ring->total)
            ring->write = ring->buf;
        goto finish_write;
    }
    /* then we should have another write */
    memcpy(ring->buf, buffer, to_write);
    ring->write = ring->buf + to_write;

 finish_write:
    /* update 'used' param finally */
    ring->used += len;
    return len;
}

unsigned int ring_buffer_read(struct ring_buffer *ring, char *buffer,
                              unsigned int len)
{
    unsigned int to_read;
    if (!buffer)
        return -1;
    len = RB_MIN(len, ring->used);
    to_read = (ring->total + 1) - (ring->read - ring->buf);
    to_read = RB_MIN(to_read, len);

    /* the first read */
    memcpy(buffer, ring->read, to_read);
    buffer += to_read;

    to_read = len - to_read;
    if (0 == to_read) {
        ring->read += len;
        if (ring->read > ring->buf+ring->total)
            ring->read = ring->buf;
        goto finish_read;
    }
    /* then we should have another read */
    memcpy(buffer, ring->buf, to_read);
    ring->read = ring->buf + to_read;

 finish_read:
    /* update 'used' param finally */
    ring->used -= len;
    return len;
}

void ring_buffer_destroy(struct ring_buffer *ring)
{
    _rb_free(ring->buf);
    _rb_free(ring);
}

#ifdef DEBUG_RING_BUFFER

void ring_buffer_dump(struct ring_buffer *ring)
{
    unsigned int total, stored, rest;
    /* how much we have in total */
    total = ring_buffer_total(ring);
    /* how much we used currently */
    stored = ring_buffer_used(ring);
    /* how much we have to store more things */
    rest = ring_buffer_rest(ring);
    dbg("%d/%d used, rest %d, wr_index %d, rd_index %d",
        stored, total, rest,
        ring->write - ring->buf,
        ring->read - ring->buf);
}

int main(void)
{
    struct ring_buffer *ring;
    char buf[1024];
    unsigned int ret, rd_nr, wr_nr, i = 10;
    /* create the buffer */
    ring = ring_buffer_create(100);
    if (!ring) {
        dbg("ERR: failed create ring");
        return -1;
    }
    ring_buffer_dump(ring);
    srand(time(NULL));

    dbg("starting test");

    while (i--) {
        rd_nr = random()%64;
        wr_nr = random()%64;
        /* write sth */
        ret = ring_buffer_write(ring, buf, wr_nr);
        if (ret < 0) {
            dbg("ERR: write failed: %d", ret);
            return -1;
        } else {
            dbg("%d/%d written", ret, wr_nr);
        }
        /* read sth. read never fail as long as the buffer is inited. */
        ret = ring_buffer_read(ring, buf, rd_nr);
        if (ret < 0) {
            dbg("ERR: read ret < 0!");
            return -1;
        } else {
            dbg("%d/%d read", ret, rd_nr);
        }
        ring_buffer_dump(ring);
    }
    dbg("test done");
}

#endif /* ifdef DEBUG_RING_BUFFER */

