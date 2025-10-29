#include "rpc/server.h"
#include "rpc/client.h"
#include "rpc/delimiter_codec.h"    //in handleClient 
#include <arpa/inet.h>              //accept(server_fd,...) and ::shutdown(server_fd,...) etc.
#include <sys/un.h>                 //sockaddr_un
#include <unistd.h>                 //close(client_fd)
#include <spdlog/spdlog.h>
#include <thread>                   //handleClient thread

namespace rpc{
    RpcServer::RpcServer(const type::PeerInfo& selfInfo){
        selfInfo_ = selfInfo;
        initSocket();
    }
    RpcServer::~RpcServer(){Stop();}
    void RpcServer::Register_Handler(
        const std::string& method,std::function<std::string(const std::string&)> handler){handlers[method] = handler;}
    /*
    1. Accept connections with accept().
    2. Communicate using send() and recv().
    3. Close the socket with close().
    */
    void RpcServer::Start() {
        running.store(true);
        //in loop to accept and handle connections
        while (running.load()) {
            //block until a new connection is accepted
            int client_fd = accept(server_fd, nullptr, nullptr);
            if(client_fd < 0) {
                if(!running){
                    // spdlog::info("[RpcServer] start() exiting accept loop");
                    // server is stopping, exit loop
                    break;
                }
                spdlog::error("[RpcServer] start() Failed to accept connection");
                //continue to stay alive
                continue;
            }
            // spdlog::info("[RpcServer] start() Accepted new connection: fd={}", client_fd);
            // Function&&(void(RpcServer::*func_ptr)(int)), this , Args&&... args
            // std::thread(&RpcServer::handleClient, this, client_fd).detach();
            std::thread t(&RpcServer::handleClient, this, client_fd);
            {
                std::lock_guard<std::mutex> lg(threads_mtx_);
                clientThreads_.emplace_back(std::move(t));
            }
        }// end of while loop
        // spdlog::info("[RpcServer] start() RpcServer stopped");
    }

    void RpcServer::Stop() {
        // spdlog::info("[RpcServer] stop() RpcServer stopping...");
        running.store(false);
        if (server_fd >= 0) {
            //close the server actively,to break the accept() blocking
            //after which server will exit while loop and return 0,server process exits normally
            // spdlog::info("[RpcServer] stop() Closing server socket...");
            //parameter:int sockfd, int how
            //how: SHUT_RD, SHUT_WR, SHUT_RDWR
            //SHUT_RDWR:disables further send and receive operations
            ::shutdown(server_fd, SHUT_RDWR);
            //decrease reference count of the socket
            ::close(server_fd);
            server_fd = -1;
        }


        // join worker threads
        // avoid join when holding the lock
        std::vector<std::thread> threadsCopy;
        {
            std::lock_guard<std::mutex> lg(threads_mtx_);
            threadsCopy.swap(clientThreads_);
        }
        for (auto &t : threadsCopy) {
            if (t.joinable()) t.join();
        }
    }
    
    //---------private functions---------

    /*
    1. Create a socket with socket(AF_UNIX, SOCK_STREAM, 0).
    2. Bind the socket to a socket file path using bind().
    3. Listen for incoming connections with listen().
    */
    void RpcServer::initSocket(){
        //AF_UNIX:UNIX domain socket
        //SOCK_STREAM:stream socket(TCP)
        server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if(server_fd < 0) {
            spdlog::error("[RpcServer] initSocket() Failed to create socket");
            return;
        }

        //sockaddr_un: UNIX domain socket
        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        //self path for server
        std::string sock_path = selfInfo_.sockPath;
        //sock_path.c_str():convert std::string to const char*
        std::strncpy(addr.sun_path, sock_path.c_str(), sizeof(addr.sun_path) - 1);
        //ensure null-termination
        addr.sun_path[sizeof(addr.sun_path)-1] = '\0';
        //cpy will copy until '\0',but sun_path is not guaranteed to be null-terminated
        //delete existing socket file,or Address already in use error
        ::unlink(sock_path.c_str()); 

        //bind() will create the socket file at sock_path automatically
        socklen_t len = offsetof(sockaddr_un, sun_path) + strlen(addr.sun_path) + 1;
        if (bind(server_fd, (struct sockaddr*)&addr, len) < 0) {
            spdlog::error("[RpcServer] initSocket() Failed to bind socket");
            close(server_fd);
            throw std::runtime_error("[RpcServer] initSocket() Failed to bind socket");
        }
        //listen for incoming connections
        if (listen(server_fd, SOMAXCONN) < 0) {
            spdlog::error("[RpcServer] initSocket() Failed to listen on socket");
            close(server_fd);
            throw std::runtime_error("[RpcServer] initSocket() Failed to listen on socket");
        }

        // spdlog::info("[RpcServer] initSocket() RpcServer listening on {}", sock_path);
    }

    void RpcServer::handleClient(int client_fd){
        rpc::DelimiterCodec codec;
        char buf[4096];
        std::string data;
        //ssize_t: signed size type
        //recv parameters: socket fd, buffer, buffer size, flags
        //flags:0 means no special options
        //:: avoids potential c library function name conflicts with member functions
        while(true){
            ssize_t n = ::recv(client_fd, buf, sizeof(buf), 0);
            //rpc is once time request-response, so only recv once
            if (n == 0) {
                // client closed normally
                // spdlog::info("[RpcServer] start() Client closed connection");
                close(client_fd);
                return;
            } else if (n < 0) {
                //signal interrupt, try again
                // if (errno == EINTR) continue;
                spdlog::error("[RpcServer] start() recv failed: {}", strerror(errno));
                close(client_fd);
                return;
            }
            data.append(buf, static_cast<size_t>(n));
            //now data should conform to the rpc message format defined in client.h
            //[methodName]\n[payload]\nEND\n
            //there could be multiple rpc messages in data
            
            while (true) {
                auto req_opt = codec.tryDecodeRequest(data);
                if (!req_opt) break;
                //req_opt is std::optional<RpcRequest>
                const auto& [method, payload] = *req_opt;

                // === call handler ===
                std::string reply_payload;
                if (handlers.count(method)) {
                    reply_payload = handlers[method](payload);
                } else {
                    reply_payload = "ERROR: unknown method";
                }

                // === encode response ===
                std::string framed = codec.encodeResponse(reply_payload);
                // spdlog::info("Sending RPC response: {}'", framed);
                ::send(client_fd, framed.data(), framed.size(), 0);
            }//do not close until all messages are processed
            data.clear();
        }
        close(client_fd);
        // spdlog::info("[RpcServer] start() RpcServer closed connection: fd={}", client_fd);
    }
}//namespace rpc