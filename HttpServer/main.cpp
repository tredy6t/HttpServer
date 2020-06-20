#include <iostream>
#include "HttpServer.h"


int main(int argc, char *argv[])
{
    int nPort = 7999;
    if (!HttpServer::Instance().Start(nPort)) {
        return -1;
    }
    std::cout << "Press any key to exit";
    getchar();
    HttpServer::Instance().Stop();
    return 0;
}