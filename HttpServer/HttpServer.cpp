#include "HttpServer.h"
#include <iostream>
#include <map>


#ifdef _WIN32
#define HTTP_LOCAL_TIME(time,tm) localtime_s(&tm, &time)
#else
#define HTTP_LOCAL_TIME(time,tm) localtime_r(&time, &tm)


#endif // _WIN32



HttpServer::HttpServer()
{
    m_bExit.store(false);
}


std::map<RequestType,std::string> g_mapUri
{
    { kRequestTypeHeartbeat , "/Heartbeat" }
};

HttpServer& HttpServer::Instance()
{
    static HttpServer g_Instance;
    return g_Instance;
}

HttpServer::~HttpServer()
{
    Stop();
}

bool HttpServer::Start(int nPort, int nThread)
{
    mg_mgr_init(&m_mgrHttp, NULL);
    std::string port = std::to_string(nPort);
    mg_connection *pConn = mg_bind(&m_mgrHttp, port.c_str(), &HttpServer::event_handle);
    if (nullptr == pConn)
    {
        std::cout << "Bind http server at port:" << nThread << " error";
        return false;
    }
    //for both http and websocket
    mg_set_protocol_http_websocket(pConn);
    init_handle(pConn);
    std::cout << "starting http server at port:" << nPort << " success\n";
    m_thRun = std::thread(&HttpServer::run, this);
    return true;
}

void HttpServer::Stop()
{
    m_bExit.store(true);
    if(m_thRun.joinable())
        m_thRun.join();
}

void HttpServer::event_handle(mg_connection *pConn, int nEvtType, void *pData)
{
    switch (nEvtType)
    {
    case MG_EV_HTTP_REQUEST: //http
    {
        event_http_handle(pConn, pData);
        break;
    }
    //websocket
    case MG_EV_WEBSOCKET_HANDSHAKE_DONE:
    case MG_EV_WEBSOCKET_FRAME:
    case MG_EV_CLOSE:
    {
        websocket_message *pWebMsg = (websocket_message*)pData;
        break;
    }
    }
}

void HttpServer::event_http_handle(mg_connection *pConn, void *pData)
{
    http_message *pHttpMsg = static_cast<http_message*>(pData);
    bool bKeepAlive = HttpServer::Instance().check_keep_alive(pHttpMsg);
    if (pHttpMsg != nullptr)
    {
        std::string strResponse;
        switch (route_check(pHttpMsg))
        {
        case kRequestTypeHeartbeat:
        {
            std::string strBody = std::string(pHttpMsg->body.p, pHttpMsg->body.len);
            HttpServer::Instance().handle_heartbeat(strBody, strResponse);
            break;
        }
        default:
        {
            mg_printf(pConn,
                "%s",
                "HTTP/1.1 501 Not Implemented\r\n" // 也可以用HTTP/2.0
                "Content-Length: 0\r\n\r\n");
            mg_printf_http_chunk(pConn, "", 0);
            return;
        }

        HttpServer::Instance().do_response(pConn, bKeepAlive, strResponse);
        }
    }
}

RequestType HttpServer::route_check(const http_message *pHttpMsg)
{
    for (const auto& iter : g_mapUri)
    {
        if (0 == mg_vcmp(&pHttpMsg->uri, iter.second.c_str()))
            return iter.first;
    }
    return kRequestTypeUnknow;
}

void HttpServer::get_data(mg_connection *pConn, int nEvtType, void *pData)
{
    http_message* pHttpMsg = static_cast<http_message*>(pData);
    bool bKeepAlive = HttpServer::Instance().check_keep_alive(pHttpMsg);
    std::string strResult = "Invalid input";
    if (pHttpMsg != nullptr)
        strResult = "Welcome visit the http server";
    HttpServer::Instance().do_response(pConn, bKeepAlive, strResult);
}

void HttpServer::calc_data(mg_connection *pConn, int nEvtType, void *pData)
{
    http_message* pHttpMsg = static_cast<http_message*>(pData);
    bool bKeepAlive = HttpServer::Instance().check_keep_alive(pHttpMsg);
    std::string strResult = "Invalid input";
    if (pHttpMsg != nullptr)
    {
        // http://ip:port/CalcData?num1=1&num2=2
        char szNum1[10] = { 0 }, szNum2[10] = { 0 };
        mg_get_http_var(&pHttpMsg->body,"num1", szNum1,sizeof(szNum1));
        mg_get_http_var(&pHttpMsg->body, "num2", szNum2, sizeof(szNum2));
        try
        {
            int nRst = std::stoi(szNum1) + std::stoi(szNum2);
            strResult = "The result is " + std::to_string(nRst);
        }
        catch (...)
        {
            strResult = "Calculate result exception";
        }
    }
    HttpServer::Instance().do_response(pConn, bKeepAlive, strResult);
   
}


void HttpServer::run()
{
    while (!m_bExit.load())
    {
        mg_mgr_poll(&m_mgrHttp, 500); // ms
    }
    mg_mgr_free(&m_mgrHttp);

}

void HttpServer::init_handle(mg_connection* pConn)
{
    // setup handlers
    mg_register_http_endpoint(pConn, "/GetData", HttpServer::get_data MG_UD_ARG(NULL));
    mg_register_http_endpoint(pConn, "/CalcData", HttpServer::calc_data MG_UD_ARG(NULL));
}

void HttpServer::handle_heartbeat(const std::string& strBody, std::string& strResult)
{
    // do nothing, just give a response
    strResult = "Welcome join in";
}

bool HttpServer::check_keep_alive(struct http_message *pHttpMsg)
{
    auto pHeader = mg_get_http_header(pHttpMsg, "Connection");
    if (nullptr == pHeader &&
        0 == mg_vcasecmp(&pHttpMsg->proto, "HTTP/1.1"))
        return true;
    else if (0 == mg_vcasecmp(pHeader, "keep-alive"))
        return true;
    return false;
}

void HttpServer::do_response(mg_connection *pConn, 
    bool bKeepAlive, 
    const std::string& strRst)
{
    mg_printf(pConn,
        "HTTP/1.1 200 OK\r\n"
        "Date:%s\r\n"
        "Content-Type: Application/json\r\n"
        "Transfer-Encoding:chunked\r\n"
        "Connection: %s\r\n"
        "Access-Control-Allow-Orign:*\r\n\r\n",
        get_curr_time().c_str(),
        bKeepAlive ? "keep-alive" : "close");
    mg_printf_http_chunk(pConn, strRst.c_str());
    mg_printf_http_chunk(pConn, "", 0);
}

std::string HttpServer::get_curr_time()
{
    time_t now = time(nullptr);
    tm tmNow = { 0 };
    HTTP_LOCAL_TIME(now, tmNow);
    char szTime[kTimeSize] = { 0 };
    strftime(szTime, kTimeSize,"%Y-%m-%d %H:%M:%S", &tmNow);
    return std::string(szTime);
}