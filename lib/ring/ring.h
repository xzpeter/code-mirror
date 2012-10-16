#ifndef __RING_H__
#define __RING_H__

extern struct ring_buffer;

/* create a buffer with size. The read size should be 1 smaller */
struct ring_buffer *ring_buffer_create(unsigned int size);
/* some GET functions. */
/* return total size of buffer */
unsigned int ring_buffer_total(struct ring_buffer *ring);
unsigned int ring_buffer_used(struct ring_buffer *ring);
unsigned int ring_buffer_rest(struct ring_buffer *ring);

/* write data BUFFE with len LEN to ring buffer RING.
   ret >= 0 if written, and value should be the length of written (can be
   smaller than LEN). ret < 0 if failed */
unsigned int ring_buffer_write(struct ring_buffer *ring, char *buffer,
                               unsigned int len);
/* merely the same as ring_buffer_write, just use read instead of write */
unsigned int ring_buffer_read(struct ring_buffer *ring, char *buffer,
                              unsigned int len);
/* destroy the buffer after use */
void ring_buffer_destroy(struct ring_buffer *ring);

#endif
