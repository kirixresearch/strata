
#include <stdio.h>
#include <io.h>

#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#define close(x) _close(x)
#endif




char response[] = "HTTP/1.1 200 OK\r\nConnection: close\r\n"
                  "Content-Type: text/html; charset=UTF-8\r\n\r\n"
                  "<html><head><title>Hello, world</title></head>"
                  "<body><h1>Hello, world!</h1></body></html>\r\n";
 
struct wsrv_context* wsrv_start()
{
    int one = 1, client_fd;
    int port;
    struct sockaddr_in svr_addr, cli_addr;
    socklen_t sin_len = sizeof(cli_addr);
    char buf[8192];
    int bytes_received;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return NULL;  // can't open socket
 
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&one, sizeof(int));
 
    port = 8080;
    svr_addr.sin_family = AF_INET;
    svr_addr.sin_addr.s_addr = INADDR_ANY;
    svr_addr.sin_port = htons(port);
 
    if (bind(sock, (struct sockaddr *) &svr_addr, sizeof(svr_addr)) == -1)
    {
        close(sock);
        return NULL;   // can't bind
    }
 
    listen(sock, 100);

    

    while (1)
    {
        client_fd = accept(sock, (struct sockaddr *) &cli_addr, &sin_len);
        printf("connection established\n");
 
        if (client_fd == -1)
        {
            perror("Can't accept");
            continue;
        }
 

        
        //bytes_received = recv(client_fd, buf, sizeof(buf), 0);
        while (recv(client_fd, buf, sizeof(buf), 0) > 0)
        {
        }



        send(client_fd, response, sizeof(response) - 1, 0); /*-1:'\0'*/

        shutdown(client_fd, SD_SEND);
        closesocket(client_fd);
    }
}

