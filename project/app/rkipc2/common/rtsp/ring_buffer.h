
#ifdef __cplusplus
extern "C"
    {
#endif

#include <sys/time.h>
#define MAX_SIZW e

struct r_buf
{
    char *data;
    int data_len;
    struct timeval tv;
};


int ring_buffer_fifo_in(char *buf, int len);

struct r_buf* ring_buffer_fifo_out();


int ring_buffer_fifo_out_finish(struct r_buf* rbuf);


#ifdef __cplusplus
    }
#endif