#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>

#include "sysdef.h"

#ifdef WIN32
#define SEPARATOR "\\"
const char local_file_separator = '\\';
#else
#define SEPARATOR "/"
const char local_file_separator = '/';
#endif

#define DEFAULT_SERVER_PORT (1121 + 9280)
#define DEFAULT_BUFFER_SIZE (1 << 20)
static char *gbuffer = NULL;

#ifndef SERVER
char remote_file_separator = '\\';
unsigned int all_trans_bytes = 0;
unsigned int all_trans_gbytes = 0;
#endif

#if 0
struct server_args {
	char trans_path[256];
	char password[256];
	int port;
	int magic;
};

static void usage(const char *arg0)
{
	fprintf(stderr, "%s", arg0);
	fprintf(stderr, "\t[-p Password]\n");
	fprintf(stderr, "\t[-P Port]\n");
	fprintf(stderr, "\t[-d Directory]\n");
	fprintf(stderr, "\t[-f File]\n");
}

static int get_opt(struct server_args *server, int argc, char *argv[])
{
	int num = argc >> 1;

	if (!server) return -1; 
	*(server->password) = '\0';
	server->port = DEFAULT_SERVER_PORT; 
	snprintf(server->trans_path, sizeof(server->trans_path), "%s", ".");
	while (num-- > 0) {
		if (!strncmp("-p", argv[num * 2], 2)) {
			snprintf(server->password, sizeof(server->password), "%s", argv[num * 2 + 1]);
		} else if (!strncmp("-P", argv[num * 2], 2)) {
			server->port = atoi(argv[num * 2 + 1]);
		} else if (!strncmp("-d", argv[num * 2], 2) || !strncmp("-f", argv[num * 2], 2)) {
			snprintf(server->trans_path, sizeof(server->trans_path), "%s", argv[num * 2 + 1]);
		}
	}
	return 0;
}
#endif
#ifdef SERVER
static int trans_file(int sock, const char *_file)
{
	char file[1024];
	int len = 0, bytes = 0;
	struct stat st;
	char *buffer = gbuffer;

	if (sock < 0) return -1;
	if (!_file) return -1;
	len = snprintf(file, sizeof file, "%s", _file);
	if (stat(file, &st)) {
		fprintf(stderr, "stat %s : %s\n", file, strerror(errno));
		return -1;
	}
	if (!S_ISREG(st.st_mode)) {
		fprintf(stderr, "%s : is not a regular file !\n", file);
		return 0;
	}
#ifdef WIN32
	int fd = open(file, O_RDONLY | O_BINARY);
#else
	int fd = open(file, O_RDONLY);
#endif
	if (fd < 0) {
		fprintf(stderr, "%s : %s\n", file, strerror(errno));
		return -1;
	}
	len = htonl(len);
	if (send(sock, (char *)&len, sizeof(int), 0) != sizeof(int)) {
		fprintf(stderr, "send file name len Failed : %s\n", strerror(errno));
		return -1;
	}
	len = ntohl(len);
	if (send(sock, file, len, 0) != len) {
		fprintf(stderr, "send file name Failed : %s\n", strerror(errno));
		return -1;
	}
	len = htonl(st.st_size);
	if (send(sock, (char *)&len, sizeof(int), 0) != sizeof(int)) {
		fprintf(stderr, "file size Failed : %s\n", strerror(errno));
		return -1;
	}
	len = st.st_size;
	while (len > 0) {
		bytes = read(fd, buffer, DEFAULT_BUFFER_SIZE);
		if (bytes <= 0 || send(sock, buffer, bytes, 0) != bytes) break;
		len -= bytes;
	}
	close(fd);

	return len ? -1: 0;
}

static int trans_dir(int sock, const char *dir)
{
	struct stat st;
	char subdir[1024];

	if (!dir) return -1;
	if (stat(dir, &st)) {
		fprintf(stderr, "stat %s : %s\n", dir, strerror(errno));
		return -1;
	}
	if (S_ISDIR(st.st_mode)) {
		struct dirent *d = NULL;
		int ret = 0;
		DIR *pdir = opendir(dir);
		if (!pdir) {
			fprintf(stderr, "dir : %s : %s\n", dir, strerror(errno));
			return -1;
		}
		while ((d = readdir(pdir))) {
			if (!strcmp(d->d_name, "..") || !strcmp(d->d_name, ".")) continue;
			snprintf(subdir, sizeof(subdir), "%s"SEPARATOR"%s", dir, d->d_name);
			ret = trans_dir(sock, subdir);
			if (ret) {
				fprintf(stderr, "Subdir : %s\n", subdir);
				break;
			}
		}
		closedir(pdir);
		return ret;
	} else if (S_ISREG(st.st_mode)) {
		fprintf(stderr, "Delivering file: %s\n", dir);
		return trans_file(sock, dir);
	}
	fprintf(stderr, "%s is not a regular file or a directory !", dir);

	return -1;
}
#else

static int change_file_separator(char *file)
{
	while (*file) {
		if (*file == remote_file_separator) {
			*file = local_file_separator;
		}
		file++;
	}
	return 0;
}

static int dir_tree_build(const char *dir)
{
	char parent_dir[256];
	char *p = NULL;
	int offset = 0;

	while ((p = strchr(dir + offset, local_file_separator))) {
		offset = p - dir;
		strncpy(parent_dir, dir, offset);
		parent_dir[offset] = '\0';
		if (access(parent_dir, F_OK) && mkdir(parent_dir, 0774)) {
			fprintf(stderr, "mkdir Failed : %s\n", strerror(errno));
			return -1;
		}
		offset++;
	}

	if (access(dir, F_OK)) {
		return mkdir(dir, 0774);
	}

	return 0;
}

static int trans_dir_client(int sock)
{
	char file[1024];
	int len = 0, bytes = 0;
	char *buffer = gbuffer;

	if (sock < 0) return -1;
	if (recv(sock, (char *)&len, sizeof(int), 0) != sizeof(int)) {
		fprintf(stderr, "recv file name len Failed : %s\n", strerror(errno));
		return -1;
	}
	len = htonl(len);
	if (len <= 0) {
		fprintf(stderr, "The End !\n");
		return 1;
	}
	if (recv(sock, file, len, 0) != len) {
		fprintf(stderr, "recv file name Failed : %s\n", strerror(errno));
		return -1;
	}
	*(file + len) = '\0';
	// dir tree build
	if (local_file_separator != remote_file_separator) {
		change_file_separator(file);
	}
	char *p = strrchr(file, local_file_separator);
	if (p) {
		char dir[256];
		strncpy(dir, file, p - file);
		dir[p - file] = '\0';
		dir_tree_build(dir);
	}
#ifdef WIN32
	int fd = open(file, O_RDWR | O_TRUNC | O_CREAT | O_BINARY, 0644);
#else
	int fd = open(file, O_RDWR | O_TRUNC | O_CREAT, 0644);
#endif
	if (fd < 0) {
		fprintf(stderr, "%s : %s\n", file, strerror(errno));
		return -1;
	}
	if (recv(sock, (char *)&len, sizeof(int), 0) != sizeof(int)) {
		fprintf(stderr, "file size Failed : %s\n", strerror(errno));
		close(fd);
		return -1;
	}
	// FIXME if the file larger than 4G
	len = htonl(len);
	all_trans_bytes += len;
	while (len > 0) {
		bytes = recv(sock, buffer, DEFAULT_BUFFER_SIZE >= len ? len : DEFAULT_BUFFER_SIZE, 0);
		if (bytes <= 0 || bytes != write(fd, buffer, bytes)) break;
		len -= bytes;
	}
	close(fd);

	return len ? -1: 0;
}
#endif

#ifdef SERVER
static int tcp_listen(int port)
{
	int sock;
	int one = 1;
	struct sockaddr_in addr;

#ifdef Win32
	WSADATA wsaData;
	int rc = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (rc != 0) {
		return -1;
	}
#endif

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;

	if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		fprintf(stderr, "create socket fail\n");
		return -1;
	}

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one)) < 0) {
		fprintf(stderr, "setsockopt fail\n");
		close(sock);
		return -1;
	}

	if (bind(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
		fprintf(stderr, "bind fail\n");
		close(sock);
		return -1;
	}

	if (listen(sock, 1) < 0) {
		close(sock);
		return -1; 
	}

	return sock;
}
#else
#ifdef Win32
static void cleanup_winsock(void)
{
	WSACleanup();
}
#endif

int conn_to_serv(char *ip, unsigned int port)
{
	int sock;
	struct sockaddr_in addr;

	if (!ip) ip = "127.0.0.1";
	if (port > 0xffff || port < 1000)
		port = DEFAULT_BUFFER_SIZE;
#ifdef Win32
	WSADATA wsaData;
	int rc = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (rc != 0) {
		return -1;
	}
	atexit(cleanup_winsock);
#endif

	memset(&addr, 0, sizeof(addr));
	addr.sin_addr.s_addr = inet_addr(ip);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) == -1) {
		fprintf(stderr, "Create sock fail\n");
		return -1;
	}
	if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		fprintf(stderr, "connect fail\n");
		close(sock);
		return -1;
	}
	return sock;
}
#endif

/*
int get_client(int port)
{
	int listen_sock;
	struct sockaddr_in clientaddr;

	listen_sock = tcp_listen(DEFAULT_SERVER_PORT);
	if (listen_sock < 0) {
		fprintf(stderr, "listen Failed\n");
		return -1;
	}
	socklen_t alen = sizeof(struct sockaddr_in);

	return accept(listen_sock, (struct sockaddr *)&clientaddr, &alen);
}
*/

#ifdef SERVER
int trans_info(int sock)
{
	char info[4] = {0, 0, 0, 0};

	info[3] |= local_file_separator;
	if (send(sock, info, sizeof(info), 0) != sizeof(info)) {
		fprintf(stderr, "%s : send info fail %s\n", __func__, strerror(errno));
		return -1;
	}
	return 0;
}

int main(int argc, char *argv[])
{
	//if (get_opt(&server_args, argc, argv)) {
	//usage(argv[0]);
	//return 1;
	//}

	int listen_sock;
	struct sockaddr_in clientaddr;

	listen_sock = tcp_listen(DEFAULT_SERVER_PORT);
	if (listen_sock < 0) {
		fprintf(stderr, "listen Failed\n");
		return -1;
	}
	socklen_t alen = sizeof(struct sockaddr_in);

	int sock = accept(listen_sock, (struct sockaddr *)&clientaddr, &alen);
	//int sock = get_client(DEFAULT_SERVER_PORT);
	if (sock < 0) {
		fprintf(stderr, "Can not get client\n");
		close(listen_sock);
		return 1;
	}
	// info Trans
	if (trans_info(sock)) {
		close(sock);
		close(listen_sock);
		return 1;
	}
	gbuffer = malloc(DEFAULT_BUFFER_SIZE);
	if (!gbuffer) {
		close(sock);
		fprintf(stderr, "malloc : %s\n", strerror(errno));
		return -1;
	}
	if (trans_dir(sock, argv[1])) {
		fprintf(stderr, "trans_file fail\n");
	}
	if (gbuffer) free(gbuffer);
	// Tell the client the end meet 1
	int e = 0;

	if (send(sock, (char *)&e, sizeof(int), 0) != sizeof(int)) {
		fprintf(stderr, "send End Flag Failed : %s\n", strerror(errno));
	}
	close(sock);
	close(listen_sock);
	fprintf(stderr, "Successfully\n");

	return 0;
}
#else

int trans_info(int sock)
{
	char info[4] = {0, 0, 0, 0};

	if (recv(sock, info, sizeof(info), 0) != sizeof(info)) {
		fprintf(stderr, "%s : recv info fail %s\n", __func__, strerror(errno));
		return -1;
	}
	remote_file_separator = info[3];
	return 0;
}

int main(int argc, const char *argv[])
{
	struct timeval tv_start, tv_end;
	//int sock = conn_to_serv(NULL, DEFAULT_SERVER_PORT);
	int sock = conn_to_serv("192.168.1.105", DEFAULT_SERVER_PORT);

	if (sock < 0) {
		fprintf(stderr, "Can not connect to server\n");
		return 1;
	}
	// info Trans
	if (trans_info(sock)) {
		close(sock);
		return 1;
	}

	gettimeofday(&tv_start, NULL);
	gbuffer = malloc(DEFAULT_BUFFER_SIZE);
	if (!gbuffer) {
		close(sock);
		fprintf(stderr, "malloc : %s\n", strerror(errno));
		return -1;
	}
	while (!trans_dir_client(sock));
	close(sock);
	if (gbuffer) free(gbuffer);
	gettimeofday(&tv_end, NULL);
	fprintf(stderr, "Trans : %d MB\n", all_trans_bytes >> 20);
	fprintf(stderr, "Times : %d s\n", (int)(tv_end.tv_sec - tv_start.tv_sec));
	fprintf(stderr, "Trans : %0.2f KB/s\n", ((double)all_trans_bytes / 1024.0)/((double)(tv_end.tv_sec - tv_start.tv_sec)));

	return 0;
}
#endif
