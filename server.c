#include <unistd.h>
#include <assert.h>//assert()
#include <stdio.h>
#include <string.h>//strlen()
#include <sys/types.h>
#include <sys/socket.h>//bind() socket() accept()
#include <arpa/inet.h>//htons()

#include <event.h>
#include <event2/util.h>

#define PORT 8080
#define BACKLOG 5 //max listen connection
#define MEM_SIZE 1024

struct event_base * base;

int server_init(int port, int listen_num);
void on_read_client(int fd, short events, void* arg);
void on_accept(int fd, short events, void *arg);

int server_init(int port, int listen_num)
{
	int errno_save;
	evutil_socket_t listen_fd;

	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	assert(listen_fd != -1);

	//allow bind same addr for more than one time	
	evutil_make_listen_socket_reuseable(listen_fd);

	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = 0;
	sin.sin_port = htons(port);

	if( bind(listen_fd, (struct sockaddr*)&sin, sizeof(sin)) < 0 )
		goto error;
	
	if( listen(listen_fd, listen_num) < 0 )
		goto error;

	//allow socket non-block
	evutil_make_socket_nonblocking(listen_fd);

	return listen_fd;

error:
	errno_save = errno;
	evutil_closesocket(listen_fd);
	errno = errno_save;

	return -1;
}

void on_accept(int fd, short events, void *arg)
{
	evutil_socket_t sock_fd;

	struct sockaddr_in client;
	socklen_t len = sizeof(client);

	//accept client socket
	sock_fd = accept(fd, (struct sockaddr*)&client, &len);
	assert(sock_fd != -1);

	evutil_make_socket_nonblocking(sock_fd);

	printf("accept a client %d...\n", sock_fd);

	struct event_base* base = (struct event_base*)arg;

	struct event* ev = event_new(NULL, -1, 0, NULL, NULL);
	event_assign(ev, base, sock_fd, EV_READ | EV_PERSIST, on_read_client, (void*)ev);

	//add event
	event_add(ev, NULL);	

}

void on_read_client(int fd, short events, void* arg)
{
	char msg[1024];
	struct event* ev = (struct event*)arg;
	int len = read(fd, msg, sizeof(msg)-1);
	
	if(len <= 0){
		printf("error on read\n");
		event_free(ev);
		close(fd);
		return;
	}

	msg[len] = '\0';
	printf("recv (%d Bytes):%s\n", len, msg);

	char reply_msg[2048];

	//fill reply msg
	sprintf(reply_msg, "you said:%s", msg);

	write(fd, reply_msg, strlen(reply_msg));
}

int main(int argc, void** argv)
{
	evutil_socket_t listener = server_init(PORT, BACKLOG);	

	//new a event base
	base = event_base_new();

	struct event* ev_listen = event_new(base, listener, EV_READ|EV_PERSIST, on_accept, base);

	event_add(ev_listen, NULL);

	//start event loop
	event_base_dispatch(base);

	return 0;
}

