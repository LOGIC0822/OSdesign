#ifndef _TCP_BUFFER_
#define _TCP_BUFFER_

#define TCP_BUF_SIZE 4096

typedef struct tcp_buffer {
    int read_index;
    int write_index;
    char buf[TCP_BUF_SIZE];
    // char *buf_ptr;
} tcp_buffer;

tcp_buffer *init_buffer();
void adjust_buffer(tcp_buffer *buf);
void recycle_write(tcp_buffer *buf, int len);
void recycle_read(tcp_buffer *buf, int len);
void send_to_buffer(tcp_buffer *buf, const char *s, int len);
int buffer_input(tcp_buffer *buf, int sockfd);
void buffer_output(tcp_buffer *buf, int sockfd);

#endif
