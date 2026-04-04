#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>

void* worker(void *arg){
    int cfd=*(int*)arg;

    //通信
    while(1){
        char buff[1024];
        int len=recv(cfd,buff,sizeof(buff),0);//接收数据
        if(len>0){
            buff[len]='\0';
            std::cout<<"客户端"<<buff<<::std::endl;
            send(cfd,buff,len,0);//发送数据
        }
        else if(len==0){
            std::cout<<"客户端已经断开"<<std::endl;
            break;
        }
        else{
            perror("recv");
            break;
        }
    }
    //关闭
    close(cfd);
}


int main(){
    //创建监听的套接字,文件描述符
    int fd=socket(AF_INET,SOCK_STREAM,0);//使用ipv4，流式传输，默认的协议
    if(fd==-1){
        perror("socket");
        return -1;
    }

    // 传入参数,要绑定的IP和端口信息,建立连接的客户端的地址信息
    struct sockaddr_in saddr;
    saddr.sin_family=AF_INET;
    saddr.sin_port=htons(9999);//端口号，小段变大段(16)
    saddr.sin_addr.s_addr=INADDR_ANY;//读取实际的ip地址.  0.0.0.0
    //将文件描述符和本地的IP与端口进行绑定   
    int ret=bind(fd,(sockaddr *)&saddr,sizeof(saddr));
    if(ret==-1){
        perror("bind");
        return -1;
    }

    // 给监听的套接字设置监听
    ret=listen(fd,128);// 128为同时能处理的最大连接要求
    if(ret==-1){
        perror("listen");
        return -1;
    }

    // 传出参数,建立连接的客户端的地址信息
    struct sockaddr_in caddr;
    socklen_t addrlen=sizeof(caddr);
    //等待并接受客户端的连接请求, 建立新的连接, 会得到一个新的文件描述符(通信的)	
    while(1){
        int cfd=accept(fd,(sockaddr *)&caddr,&addrlen);
        if(cfd==-1){
            perror("accept");
            break;
        }
        //打印客户端的ip和端口信息
        char ip[32];
        std::cout<<"客户端的ip"<<inet_ntop(AF_INET,&caddr.sin_addr.s_addr,ip,sizeof(ip))//将大端的整形数, 转换为小端的点分十进制的IP地址
                 <<"端口"<<ntohs(caddr.sin_port)//大端变小段
                 <<std::endl;

        //多线程
        pthread_t thr;
        pthread_create(&thr,nullptr,worker,&cfd);
        pthread_detach(thr);
    }	

    //关闭
    close(fd);

    return 0;
}
