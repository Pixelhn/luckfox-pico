#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include "ring_buffer.h"


#ifdef __cplusplus
extern "C"
    {
#endif
#define NUM 20
struct r_buf g_num[NUM];

int total_size;
int in;
int out;


int lost;
int num;

int ring_buffer_init()
{
    total_size = 0;
    in = 0;
    out = 0;

    return 0;
}

void ring_buffer_info()
{
    printf("info in   %d-%d\n",  in, in %NUM);
    printf("info out  %d-%d\n\n",  out, out %NUM);
    printf("info lost %d\n", lost);
    printf("info size %d\n", total_size);
    printf("info num  %d\n", num);
    return ;
}


int ring_buffer_fifo_in(char *buf, int len)
{
    in++;

    if (in > out + NUM)
    {
        struct r_buf *p = ring_buffer_fifo_out();
        printf("lost\n");
        ring_buffer_fifo_out_finish(p);
        lost++;
    }

    g_num[in%NUM].data = malloc(len);
    g_num[in%NUM].data_len = len;
    gettimeofday(&g_num[in%NUM].tv, NULL);
    memcpy(g_num[in%NUM].data, buf, g_num[in%NUM].data_len);

    total_size += len;
    num++;
    ring_buffer_info();
    return 0;
}


struct r_buf* ring_buffer_fifo_out()
{
    int tmp_out = out + 1;

    if (tmp_out > in)
    {
        return NULL;
    }
    else
    {
        out = tmp_out;
        return &g_num[out % NUM];
    }
}

int ring_buffer_fifo_out_finish(struct r_buf* rbuf)
{
    free(rbuf->data);
    total_size -= rbuf->data_len;
    num--;

    rbuf->data = NULL;
    rbuf->data_len = 0;

    ring_buffer_info();
    return 0;
}

#ifdef __cplusplus
}
#endif




#if  0

int main()
{
    int i;
    printf("%p\n", ring_buffer_fifo_out());

    for (i = 0; i < 100; i++)
    {
        ring_buffer_fifo_in((char *)&i, sizeof(int));    
    }

    printf("total %d\n", total_size);

    for (; ;)
    {
        struct r_buf *p = ring_buffer_fifo_out();
        if (p  != NULL)
        {
            printf("%d\n", *(int *)p->data);
            ring_buffer_fifo_out_finish(p);
        }
        else
        {
            printf("no more\n");
            break;
        }
    }

    printf("total %d\n", total_size);
}


#endif  