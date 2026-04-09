#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <cstring>
#include <fcntl.h>

struct Node
{
    int epfd;
    int curfd;
};

void *acceptConn(void *arg)
{
    Node *node = (Node *)arg;
    int curfd = node->curfd;
    int epfd = node->epfd;

    int cfd = accept(curfd, nullptr, nullptr);
    // 读写都变成了非阻塞模式
    int flag = fcntl(cfd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(cfd, F_SETFL, flag);

    struct epoll_event ev;
    ev.events = EPOLLIN| EPOLLET; // 读缓冲区是否有数据
    ev.data.fd = cfd;
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
    if (ret == -1)
    {
        perror("epoll_ctl");
        return nullptr;
    }
    delete (node);
    return nullptr;
}

void *communication(void *arg)
{
    Node *node = (Node *)arg;
    int curfd = node->curfd;
    int epfd = node->epfd;

    char buf[5];
    char buff[1024];
    buff[0] = '\0';
    memset(buf, 0, sizeof(buf));
    while (1)
    {
        int len = read(curfd, buf, sizeof(buf));
        if (len == 0)
        {
            std::cout << "客户端已断开" << std::endl;
            epoll_ctl(epfd, EPOLL_CTL_DEL, curfd, nullptr);
            close(curfd);
            break;
        }
        else if (len > 0)
        {
            write(STDOUT_FILENO, buf, len);
            std::cout << std::endl;
            strncat(buff, buf, len);
        }
        else
        {
            if (errno == EAGAIN)
            {
                std::cout << "读完了" << std::endl;
                write(curfd, buff, strlen(buff));
                break;
            }
            perror("read");
            break;
        }
    }
    delete (node);
    return nullptr;
}

int main()
{
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1)
    {
        perror("socket error");
        return -1;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(9999);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    int ret = bind(lfd, (sockaddr *)&serv_addr, sizeof(serv_addr));
    if (ret == -1)
    {
        perror("bind error");
        return -1;
    }

    ret = listen(lfd, 128);
    if (ret == -1)
    {
        perror("listen error");
        return -1;
    }

    // 创建一个epoll模型
    int epfd = epoll_create(100);
    if (epfd == -1)
    {
        perror("epoll_create");
        return -1;
    }

    // 读写都变成了非阻塞模式
    int flag = fcntl(lfd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(lfd, F_SETFL, flag);

    // 往epoll实例中添加需要检测的节点, 现在只有监听的文件描述符
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET; // 读缓冲区是否有数据
    ev.data.fd = lfd;
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
    if (ret == -1)
    {
        perror("epoll_ctl");
        return -1;
    }

    struct epoll_event evs[1024];
    int size = 1024;
    pthread_t pth;
    while (1)
    {
        int num = epoll_wait(epfd, evs, size, -1);
        for (int i = 0; i < num; i++)
        {
            int curfd = evs[i].data.fd; // 取出当前的文件描述符
            Node *node = new Node;
            node->curfd = curfd;
            node->epfd = epfd;
            if (curfd == lfd)
            {
                pthread_create(&pth, nullptr, acceptConn, node);
                pthread_detach(pth);
            }
            else
            {
                pthread_create(&pth, nullptr, communication, node);
                pthread_detach(pth);
            }
        }
    }
}