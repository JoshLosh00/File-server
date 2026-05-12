#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h> 
#include <errno.h>
#include <sys/wait.h>
#include "vector.h"
#include <fcntl.h>

#define PORT "8080"
#define BACKLOG 10
#define BUFFER_SIZE 2048
#define OUTBUFFER_SIZE 2048

int shutdown_fd[2];

void senderror(int socket,const char *errormsg, const char *status){
    char header[256];

    snprintf(header, sizeof(header),
        "HTTP/1.1 %s\r\n"
        "Content-Length: %zu\r\n"
        "\r\n",status,
        strlen(errormsg));

    send(socket, header, strlen(header), 0);
    send(socket, errormsg, strlen(errormsg), 0);
}

void handle_sigchld(int sig){
    int status;
    while ((waitpid(-1,&status,WNOHANG))>0){

    }
}

void handle_sigint(int sig){
    char x = 1;
    write(shutdown_fd[1], &x, 1);
}

int main(){
    struct addrinfo hints, *servinfo;
    struct sockaddr_storage connection;
    socklen_t addr_size;
    int sfd , newsfd, t;
    char header_sent = 0;
    char header[256];
    char buf[1024];
    char *msg;
    char *filename;
    char buffer[BUFFER_SIZE];

    pipe(shutdown_fd);

    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, PORT, &hints, &servinfo) != 0) {
        printf("error");
        return 1;
    }

    sfd = socket(servinfo->ai_family,servinfo->ai_socktype,servinfo->ai_protocol);

    if (sfd == -1)  {
        perror("socket");
        return 2;
    }


    int yes =1;

    // lose the "Address already in use" error message
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

    if (bind(sfd,servinfo->ai_addr,servinfo->ai_addrlen) == -1){
        perror("bind");
        return 3;
    }

    if (listen(sfd, BACKLOG) == -1){
        perror("listen");
        return 4;
    }

    struct sigaction sa;
    sa.sa_handler = handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    struct sigaction sa_shutdown;
    sa_shutdown.sa_handler = handle_sigint;
    sigemptyset(&sa_shutdown.sa_mask);
    sa_shutdown.sa_flags = 0;

    sigaction(SIGCHLD, &sa, NULL);
    sigaction(SIGINT, &sa_shutdown, NULL);


    struct pollfd p_shutdown_fd = {
        shutdown_fd[0],
        POLLIN,
        0
    };

    struct pollfd p_sfd = {
        sfd,
        POLLIN,
        0
    };

    struct pvector v;
    pvector_init(&v,4);

    struct pollfd *fds = malloc(2 * sizeof(struct pollfd)); 

    fds[0] = p_shutdown_fd;
    fds[1] = p_sfd;
    size_t nfds = v.size + 2;
    size_t nfds_cap = 4;//initial value
    /*struct pollfd fds[2];
    fds[0] = p_shutdown_fd;
    fds[1] = p_sfd;*/

    while(1){

        //Building the structure for poll.
        if(nfds < v.size + 2){
            if(nfds_cap < v.capacity + 2){
                nfds_cap  = v.capacity * 2;
                struct pollfd *tmp_fds = realloc(fds, nfds_cap * sizeof(struct pollfd));
                if(tmp_fds == NULL){
                    printf("Malloc error\n");
                    continue;
                }
                fds = tmp_fds;
            }
            for (int i = 0; i < v.size; i++){
                fds[i+2] = v.data[i].poll;
            }
            nfds = v.size + 2;
        }

        int p = poll(fds, v.size + 2, -1);

        if (p <= 0){
            if(errno != EINTR){
                printf("Poll error.");
            }
            continue;
        }

        if(fds[0].revents & POLLIN){
            freeaddrinfo(servinfo);
            pvector_free(&v);
            close(sfd);
            printf("Server shutdown.\n");
        return 0;
        }

        if(fds[1].revents & POLLIN){
            addr_size = sizeof(connection);
            newsfd= accept(sfd, (struct sockaddr *)&connection, &addr_size);

            if (newsfd == -1){
                perror("accept");
                continue;
            }

            fcntl(newsfd, F_SETFL, O_NONBLOCK);

            struct pollfd new_pollfd = {
                newsfd,
                POLLIN,
                0
            };

            char *tmp = malloc(256);
            if(tmp == NULL){
                printf("Malloc error.\n");
                continue;
            }

            char *tmp2 = malloc(OUTBUFFER_SIZE);
            if(tmp2 == NULL){
                printf("Malloc error.\n");
                continue;
            }

            struct fdinfo new_poll = {
                new_pollfd,
                tmp,//inbuffer
                0,//read
                256,//in_cap
                tmp2,//outbuffer
                0,//sent
                0,//outlength
                0,//buildmessage
                //0,//headersent
                //0,//headerlength
                NULL//file
            };

            int j = 1;

            //For reusing dead connections.
            for(int i = 0; i < v.size; i++){
                if(v.data[i].poll.fd == -1){
                    v.data[i] = new_poll;
                    j = 0;
                    fds[i+2]=v.data[i].poll;
                }
            }

            if(j){
                pvector_push(&v, new_poll);
            }
            continue;
        }

        for(int i = 0; i < v.size; i++){
        
            struct fdinfo *info = &v.data[i];

            if(fds[i+2].revents & POLLIN){// == READABLE
                int bytes = recv(info->poll.fd, buffer, BUFFER_SIZE -1, 0);

                if (bytes == -1){
                    if(errno != EAGAIN && errno != EWOULDBLOCK){
                        perror("recv");
                        freefdinfo(info);
                        continue;
                    }
                } else if(bytes == 0){
                        printf("Connection closed.\n");
                        freefdinfo(info);
                        continue;
                } else{
                    //Resizing inbuffer.
                    if(info->in_cap < info->read + bytes){
                        while(info->in_cap < info->read + bytes + 1){
                            info->in_cap *= 2;
                        }
                        char *tmp = realloc(info->inbuffer, info->in_cap);
                        if(tmp == NULL){
                            printf("Malloc error.\n");
                            continue;
                        } 
                        info->inbuffer = tmp;
                    }
                    
                    memcpy(info->inbuffer + info->read, buffer, bytes);
                    info->read += bytes;

                    info->inbuffer[info->read] = '\0';//needed so that strstr is only reading the inbuffer

                    if (strstr(info->inbuffer,"\r\n\r\n")){
                        //info->read_status = 0;
                        info->build_message = 1;
                    }
                }
            }
            
            if(info->build_message){
                msg = info->inbuffer;

                char *line = strtok(msg,"\r\n");
                printf("%s\n", line);
                char *method = strtok(line, " ");
                printf("%s\n", line);
                char *path = strtok(NULL, " ");
                printf("%s\n", path);
                if(path == NULL){
                    filename = "index.html";
                }

                if(path && strcmp(path, NULL) == 0){
                    filename = "index.html";
                } else {
                    char *slash = path ? strrchr(path, '/') : NULL;
                    filename = slash ? slash + 1 : path; 
                }

                printf("The file name is %s\n", filename);

                if(strstr(path, "..")){//I can do better than that
                    senderror(info->poll.fd,"403: forbidden.", "403 Forbidden");
                    freefdinfo2(info);
                    fds[i+2]=info->poll;
                    continue;
                }

                if(strcmp(method,"GET")){
                    printf("405 sent.\n");
                    senderror(info->poll.fd,"405: invalid operation.", "405 Method Not Allowed");
                    freefdinfo2(info);
                    fds[i+2]=info->poll;
                    continue;
                }

                info->file = fopen(filename,"rb");

                if (info->file == NULL){
                    printf("404 sent.\n");
                    senderror(info->poll.fd,"404: file not found.", "404 Not Found");
                    freefdinfo2(info);
                    fds[i+2]=info->poll;
                    continue; 
                }

                fseek(info->file, 0, SEEK_END);
                size_t size = ftell(info->file);
                rewind(info->file);

                //At present header can't be more than 256 characters long.
                snprintf(header, sizeof(header),
                "HTTP/1.1 200 OK\r\n"
                "Content-Length: %zu\r\n"
                "\r\n",
                size);

                info->out_length = strlen(header);

                memcpy(info->outbuffer, header, info->out_length);

                info->build_message = 0;
                info->poll.events = POLLOUT;
                fds[i+2].events = POLLOUT;
                printf("%s\n", info->outbuffer);
            }

            if(fds[i+2].revents & POLLOUT){
                int r = info->out_length - info->sent;
                if(r == 0){
                    info->sent = 0;
                    info->out_length = 0;
                    int t =fread(info->outbuffer, 1, OUTBUFFER_SIZE, info->file);
                    if (t == 0){//No more to read
                        printf("Message sent.\n");
                        freefdinfo(info);
                        continue;
                    }
                    info->out_length += t;
                }
                else{
                    int s = send(info->poll.fd, info->outbuffer + info->sent, r, 0);
                    if(s == -1){
                        if(errno != EAGAIN && errno != EWOULDBLOCK){
                            perror("send");
                        }
                    } 
                    else{
                        info->sent += s;
                    }
                }
            }
        }
    }
}
