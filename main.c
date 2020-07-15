
#ifdef WIN32
#define  _WIN32_WINNT 0x0600
#include <winsock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#elif 1
#include <sys/socket.h>
#include <netinet/in.h>
#endif
#include <stdio.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#define PORT 9999
#define SERVER_IP "127.0.0.1"
#define _BACKLOG_ 10

#ifdef WIN32
void init_socket() {
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA wsaData;
    if (WSAStartup(sockVersion, &wsaData) != 0) {
        return;
    }
}
int writesocket(int s, const char *buf, int len, int flags) {
    return send(s, buf, len, flags);
}
int readsocket(int s, char *buf, int len, int flags) {
    return recv(s, buf, len, flags);
}
void close_socket(int s) {
    closesocket(s);
    WSACleanup();
}
#elif 1
void init_socket() {
}
int writesocket(int s, const char *buf, int len, int flags) {
    return write(s, buf, len);
}
int readsocket(int s, char *buf, int len, int flags) {
    return read(s, buf, len);
}
void close_socket(int s) {
    close(s);
}
#endif


void server_entry()
{
    init_socket();
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0)
    {
        printf("socket error !");
        return;
    }
    struct sockaddr_in server_socket;
    struct sockaddr_in client_socket;
    memset(&server_socket, 0, sizeof(server_socket));
    server_socket.sin_family = AF_INET;
    server_socket.sin_port = htons(PORT);
    server_socket.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(sock, (struct sockaddr *)&server_socket, sizeof(struct sockaddr_in)) < 0)
    {
        printf("bind error,error code is %d,error string is :%s\n",errno,strerror(errno));
        close(sock);
        return;
    }
    if(listen(sock, _BACKLOG_) < 0)
    {
        printf("listen error,error code is %d,error string is %s\n",errno,strerror(errno));
        close(sock);
        return;
    }
    printf("bind and listen success,wait accept...\n");

    for(;;)
    {
        socklen_t len = sizeof(struct sockaddr);
        int client_sock = accept(sock, (struct sockaddr *)&client_socket, &len);
        if(client_sock < 0)
        {
            printf("accept error, error is %d,errstring is %s\n",errno,strerror(errno));
            close(sock);
            return;
        }
        char buf_ip[INET_ADDRSTRLEN];
        memset(buf_ip,'\0',sizeof(buf_ip));
        inet_ntop(AF_INET,&client_socket.sin_addr,buf_ip,sizeof(buf_ip));
        printf("get connect,ip is %s,port is %d\n",buf_ip,ntohs(client_socket.sin_port));
        char buf[1024];
        memset(buf,'\0',sizeof(buf));//跟前面的初始化对比
        readsocket(client_sock, buf, sizeof(buf), 0);
        printf("client :# %s\n", buf);

        FILE *fp;
        fp = fopen(buf, "rb");
        fseek( fp , 0 , SEEK_END);
        int size = ftell(fp);
        writesocket(client_sock, (const char *)&size,  sizeof(int), 0);
        fseek( fp , 0 , SEEK_SET);
        char buffer[4096];
        int totalbytes = 0;
        while(!feof(fp)){
            len = fread(buffer, 1, sizeof(buffer), fp);
            int bflen = writesocket(client_sock, buffer, len, 0);
            totalbytes += len;
            printf("write in bytes:%d, total written bytes:%d\n", bflen, totalbytes);
        }
        printf("write done, total bytes:%d\n", totalbytes);
    }
    close(sock);
}

void client_entry(const char *fileName)
{

    init_socket();
    struct sockaddr_in server_sock;
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    memset(&server_sock, 0, sizeof(server_sock));
    server_sock.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP, &server_sock.sin_addr);
    server_sock.sin_port = htons(PORT);

    int ret = connect(sock, (struct sockaddr*)&server_sock, sizeof(server_sock));
    if(ret < 0)
    {
        printf("connect failed...,errno is %d,errstring is %s\n",errno,strerror(errno));
    }
    printf("connect sercer success !\n");
    writesocket(sock, fileName, strlen(fileName), 0);
    char localFileName[100] = "./zol/game/maroon/";
    strcat(localFileName, "maroon.rcc");
    FILE *pf;
    pf = fopen(localFileName, "w+");
    fclose(pf);
    pf = fopen(localFileName, "wb");
    int tl = 0;
    int timeout = 3000;
    //设置接收超时时间
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(int));
    int i = 0;
    int filesize = 0;
    readsocket(sock, (char*)&filesize, sizeof(int), 0);
    printf("get file size:%d\n", filesize);
    while(1) {
        char recvBuf[4096] = { 0 };
        if(tl >= filesize)
            break;
        int len = readsocket(sock, recvBuf, 4096, 0);
        if(len <= 0)
            break;

        tl += len;
        printf("recv rcc part:%d, total bytes:%d\n", len, tl);
        fwrite(recvBuf, len, 1, pf);
    }
    printf("recv done\n");
    fclose(pf);
    close_socket(sock);

}

int main() {
    //server_entry();
    //client_entry("你的文件名称");
    return 0;
}
