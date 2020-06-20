#pragma once
#include <string>
#include <atomic>
#include <thread>
#include "../include/mongoose.h"


static const int kTimeSize = 128;
enum RequestType
{
    kRequestTypeUnknow = -1,
    kRequestTypeHeartbeat,
};
class HttpServer
{
private:
    HttpServer();
public:
    static HttpServer& Instance();
    ~HttpServer();
    bool Start(int nPort, int nThread = 1);
    void Stop();

private:
    static void event_handle(mg_connection *pConn, int nEvtType, void *pData);
    static void event_http_handle(mg_connection *pConn, void *pData);
    static RequestType route_check(const http_message *pHttpMsg);
    static void get_data(mg_connection *pConn, int nEvtType, void *pData);
    static void calc_data(mg_connection *pConn, int nEvtType, void *pData);
    

private:
    void run();
    void init_handle(mg_connection* pConn);
    void handle_heartbeat(const std::string& strBody, std::string& strResult);
    bool check_keep_alive(struct http_message *pHttpMsg);
    void do_response(mg_connection *pConn,
        bool bKeepAlive, 
        const std::string& strRst);
    std::string get_curr_time();

private:
    mg_serve_http_opts m_optsHttp;
    mg_mgr m_mgrHttp;          // 连接管理器
    std::atomic_bool m_bExit;
    std::thread m_thRun;
};