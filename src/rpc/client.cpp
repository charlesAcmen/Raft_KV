#include "rpc/client.h"
#include <string>
#include <stdexcept>    //throw in initSocket
#include <unistd.h>     // write, read
#include <sys/socket.h> // send, recv
#include <cstring>      //c_str()
#include <sys/un.h>     //struct sockaddr_un
#include <spdlog/spdlog.h>  //logging
#include "rpc/delimiter_codec.h"    //encode and decode
namespace rpc{
    RpcClient::RpcClient(const type::PeerInfo& selfInfo,
              const type::PeerInfo& targetInfo):
              selfInfo_(selfInfo),targetInfo_(targetInfo){
        initSocket();
    }
    RpcClient::~RpcClient(){
        Close();
    }
    bool RpcClient::Connect(){
        if(connected_.load(std::memory_order_acquire)) return true;
        //client address structure:sockaddr_un,unix,used in IPC
        struct sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        //target path for client
        std::string sock_path = targetInfo_.sockPath;
        //c_str():convert c++ string to c str to fit strncpy
        std::strncpy(addr.sun_path, sock_path.c_str(), sizeof(addr.sun_path) - 1);
        //ensure null-termination
        addr.sun_path[sizeof(addr.sun_path)-1] = '\0';
        //do not unlink the socket file here
        //because the server may not be running yet


        int attempts = 0;        
        while (true) {
            // offsetof:relative offset of sun_path field to the start of struct sockaddr_un
            // +1 is for '\0' at the end
            socklen_t len = offsetof(struct sockaddr_un, sun_path) + strlen(addr.sun_path) + 1;
            // spdlog::info("[RpcClient] initSocket() RpcClient connecting to {}:{}", host, port);
            if (::connect(sock_fd, (struct sockaddr*)&addr, len) == 0) {
                // success
                // spdlog::info("[RpcClient] initSocket() RpcClient connected to {}:{} at attempt {}", host, port, attempts + 1);
                connected_.store(true,std::memory_order_release);
                return true;
            } 
            if (++attempts > MAX_RETRIES) {
                // spdlog::error("[RpcClient] initSocket() RpcClient failed to connect to {}:{} after {} attempts: {}", host, port, attempts, strerror(e));
                ::close(sock_fd);
                // throw std::runtime_error(std::string("[RpcClient] initSocket() RpcClient::connect() failed: ") + strerror(e));
                connected_.store(false,std::memory_order_release);
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_INTERVAL_MS));
        }
        connected_.store(false,std::memory_order_release);
        return false;
    }
    void RpcClient::Close(){
        // if(!connected_.load(std::memory_order_acquire)) return;
        if (sock_fd >= 0) {
            ::close(sock_fd);
            sock_fd = -1;
        }
        connected_.store(false, std::memory_order_release);
    }
    std::string RpcClient::Call(
        const std::string& method, 
        const std::string& payload){
        // double checking locking pattern,to avoid multiple threads connecting simultaneously
        if(!connected_.load(std::memory_order_acquire)){
            std::lock_guard<std::mutex> lg(conn_mtx_);
            if(!connected_.load(std::memory_order_acquire)){
                bool success = Connect();
                if(!success)
                    return "[RpcClient] call(const std::string& ,const std::string&) ERROR: not connected and connect() failed";
            }
        }
        //use delimitercodec
        rpc::DelimiterCodec codec;

        // 1. constuct request payload
        const std::string request_payload = method + "\n" + payload;

        // 2. encode to framed message
        std::string framed = codec.encodeRequest(request_payload);
        
        // 3. send request
        // send parameters:connecting fd,buff,buff size,flag
        ssize_t n = send(sock_fd, framed.c_str(), framed.size(), 0);
        if (n < 0) {
            spdlog::error("[RpcClient] call(const std::string& ,const std::string&) send() failed");
            connected_.store(false, std::memory_order_release);
            return "[RpcClient] call(const std::string& ,const std::string&) ERROR: send() failed";
        }
        
        //wait for response
        std::string buffer;
        char tmp[4096];
        while (true) {
            //block until some data is received
            ssize_t r = recv(sock_fd, tmp, sizeof(tmp), 0);
            if (r < 0) {
                spdlog::error("[RpcClient] call(const std::string& ,const std::string&) recv() failed");
                connected_.store(false, std::memory_order_release);
                return "[RpcClient] call(const std::string& ,const std::string&) ERROR: recv() failed";
            } else if (r == 0) {
                // server closed connection
                connected_.store(false);
                ::close(sock_fd);
                break;
            }
            buffer.append(tmp, static_cast<size_t>(r));

            // try decode
            auto resp = codec.tryDecodeResponse(buffer);
            if (resp) {
                return *resp; // return response payload
            }
        }

        return "[RpcClient] call(const std::string& ,const std::string&) ERROR: no response";
    }



    //---------private functions---------
    void RpcClient::initSocket(){
        //create socket used to connect to server
        //AF_UNIX:UNIX domain socket
        //SOCK_STREAM:stream socket(TCP)
        sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock_fd < 0) {
            spdlog::error("[RpcClient] initSocket() failed to create socket");
            throw std::runtime_error("[RpcClient] initSocket() failed to create socket");
        }
    }
} // namespace rpc
