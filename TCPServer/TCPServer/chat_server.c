#include "chat_server.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "../../kernel_list.h"
#include <sys/types.h>
#include <unistd.h>

#ifdef ENABLE_CHAT
struct user *user_head;
struct list_head *temp_pos;//临时变量 temp variate
struct user *temp_p;//临时变量 temp variate
#endif

int Chat_Server_init(char *Server_ip, char *Server_port, int recv_max)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1)
    {
        perror("create server socket failed!\n");
        exit(0);
    }


    memset(&addr, 0, len);

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(Server_ip);
    addr.sin_port = htons(atoi(Server_port));


    if(bind(socket_fd, (struct sockaddr *)&addr, len) == -1)
    {
        perror("bind socket failed!\n");
        exit(0);
    }

    //listen socket and set max connect number equal 5
    if (listen(socket_fd, recv_max) != 0)
    {
        perror("Failed to listen socket");
        exit(0);
    }

    //initial user list
    user_head = calloc(1, sizeof(struct user) );
    if (user_head != NULL)
    {
        INIT_LIST_HEAD(&user_head->list);
    }

    return socket_fd;

}

int chat_welcome(struct user *newone)
{
    list_for_each(temp_pos, &user_head->list)
    {
        temp_p = list_entry(temp_pos, struct user, list);

        char buf[50];
        memset(buf, 0, sizeof(char)*50);
        //send ID to new user
        if (temp_p->ID == newone->ID)
            snprintf(buf, 50, "你的ID是：%d\n", newone->ID);
        //notify all users that is a new user is login
        else
            snprintf(buf, 50, "用户:%d上线了!\n", newone->ID);

        write(temp_p->connfd, buf, strlen(buf));

    }
    return 0;
}

fd_set rset;//read ready socket flag
fd_set wset;//write ready socket flag
fd_set eset;//error ready socket flag
int ID = 1;



int chat_recv_process(int socket_fd)
{

    int read_return = 0;
    char chat_buf[100];
    int maxfd = 0;
    //1.reset rset wset eset
    FD_ZERO(&rset);
    FD_ZERO(&wset);
    FD_ZERO(&eset);

    maxfd = socket_fd;
    //bind socket_fd to rset ,when new connect coming then rset will set
    //把socket_fd 置入 rset集合,当有新的连接到来时,相应绑定的rset位会set
    FD_SET(socket_fd, &rset);

    list_for_each(temp_pos,&user_head->list)
    {
        temp_p = list_entry(temp_pos, struct user, list);
        FD_SET(temp_p->connfd, &rset);
        maxfd = (maxfd > temp_p->connfd) ? maxfd : temp_p->connfd;
    }

    //2.
    select(maxfd+1, &rset, &wset, &eset, NULL);

    //a.judge whether there is a new connection
    socklen_t len = 0;
    if (FD_ISSET(socket_fd, &rset))
    {
        //get a new connection,create a new user list node
        struct user *newone = calloc(1, sizeof(struct user));
        len = sizeof(newone->addr_in);
        newone->connfd = accept(socket_fd, (struct sockaddr *)&newone->addr_in, &len);

        if (newone->connfd == -1)
        {
            perror("Failed to accept socket");
            exit(0);
        }

        //put the new user list node to tail
        list_add_tail(&newone->list, &user_head->list);

        //distribute user id
        newone->ID = ID;
        ID++;

        //notify all users that a new user has joined
        //send welcome and ID to new user
        chat_welcome(newone);

        //chat_server show message to screen
        printf("[%s:%hu]上线了\n", inet_ntoa(newone->addr_in.sin_addr), ntohs(newone->addr_in.sin_port));
    }
    //b.judge whether there is user send massage
    //and judge if someuser had offline
    struct list_head *temp_n;
    list_for_each_safe(temp_pos, temp_n, &user_head->list)
    {
        temp_p = list_entry(temp_pos, struct user, list);

        //1.判断每个用户是否已发来数据
        //judge whether there is user send massage
        if ( FD_ISSET(temp_p->connfd, &rset) )//rset状态有变化 rset state had been changed
        {

            read_return = 0;
            memset(chat_buf, 0, sizeof(char)*100);
            read_return = read(temp_p->connfd, chat_buf, 100);

            //a.收到消息 receive massage
            if (read_return > 0)
            {
                //群发消息
                broadcastMsg(temp_p, chat_buf);
                printf("已广播用户%d发来消息:%s",temp_p->ID, chat_buf);
            }
            //b.没收到消息,有人下线了 get none massage, someone had offline
            if (read_return == 0)
            {
                printf("用户%d:下线了!\n", temp_p->ID);
                close(temp_p->connfd);
                list_del(&temp_p->list);
                free(temp_p);
            }


        }


    }
    return 0;
}
//群发消息
int broadcastMsg(struct user *user_send, char *chat_buf)
{
    char chat_temp_buf[100];
    list_for_each(temp_pos, &user_head->list)
    {
        temp_p = list_entry(temp_pos, struct user, list);
        if (temp_p->ID != user_send->ID)
        {
            memset(chat_temp_buf, 0, sizeof(char)*100);
            snprintf(chat_temp_buf, 100,"用户%d发来消息:%s",user_send->ID, chat_buf);
            write(temp_p->connfd, chat_temp_buf, strlen(chat_temp_buf));
        }

    }
    return 0;

}
