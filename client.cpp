#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
int main(){
    //创建监听的套接字,文件描述符
    int fd=socket(AF_INET,SOCK_STREAM,0);//使用ipv4，流式传输，默认的协议
    if(fd==-1){
        perror("socket");
        return -1;
    }

    // 传入参数,要绑定的IP和端口信息,要连接的服务器端的地址信息
    struct sockaddr_in saddr;
    saddr.sin_family=AF_INET;
    saddr.sin_port=htons(9999);//端口号，小段变大段(16)
    inet_pton(AF_INET,"127.0.0.1",&saddr.sin_addr.s_addr);

    int ret=connect(fd,(sockaddr *)&saddr,sizeof(saddr));
    if(ret==-1){
        perror("connect");
        return -1;
    }

    //通信
    int number=0;
    while(1){
        std::string str="hellwold";
        send(fd,str.c_str(),str.size(),0);//发送数据

        char buff[1024];
        int len=recv(fd,buff,sizeof(buff),0);//接收数据
        if(len>0){
            buff[len]='\0';
            std::cout<<"服务器"<<buff<<::std::endl;
        }
        else if(len==0){
            std::cout<<"服务器已经断开"<<std::endl;
            break;
        }
        else{

            perror("recv");
            break;
        }
        sleep(1);
    }

    //关闭
    close(fd);

    return 0;
}
