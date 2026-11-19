/**
 * ============================================================================
 * FILE: backend/httplib.h
 * PURPOSE: Lightweight single-header HTTP server for the BCA Minor Project
 * 
 * Provides an httplib-compatible Server interface (matching cpp-httplib API)
 * with zero external dependencies. Works out-of-the-box on:
 * - Windows (MinGW 32-bit & 64-bit, MSVC) with Winsock2 (-lws2_32)
 * - Linux / macOS (GCC, Clang) with standard POSIX sockets
 * ============================================================================
 */

#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <sstream>
#include <fstream>
#include <cstring>
#include <cstdlib>

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef _WIN32_WINNT
        #define _WIN32_WINNT 0x0600
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    using socket_t = SOCKET;
    #define IS_VALIDSOCKET(s) ((s) != INVALID_SOCKET)
    #define CLOSESOCKET(s) closesocket(s)
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
    using socket_t = int;
    #define IS_VALIDSOCKET(s) ((s) >= 0)
    #define CLOSESOCKET(s) close(s)
#endif

namespace httplib {

struct Request {
    std::string method;
    std::string path;
    std::string body;
    std::map<std::string, std::string> headers;

    bool has_header(const std::string& key) const {
        return headers.find(key) != headers.end();
    }

    std::string get_header_value(const std::string& key) const {
        auto it = headers.find(key);
        if (it != headers.end()) return it->second;
        return "";
    }
};

struct Response {
    int status = 200;
    std::string body;
    std::string content_type = "text/plain";
    std::map<std::string, std::string> headers;

    void set_content(const std::string& s, const char* c_type) {
        body = s;
        content_type = c_type;
    }

    void set_header(const std::string& key, const std::string& val) {
        headers[key] = val;
    }
};

using Handler = std::function<void(const Request&, Response&)>;

class Server {
private:
    std::map<std::string, Handler> get_routes_;
    std::map<std::string, Handler> post_routes_;
    std::string mount_dir_ = "./frontend";
    std::string mount_prefix_ = "/";
    bool is_running_ = false;

    std::string get_mime_type(const std::string& path) const {
        if (path.rfind(".html") != std::string::npos || path.rfind(".htm") != std::string::npos) return "text/html; charset=utf-8";
        if (path.rfind(".css") != std::string::npos) return "text/css; charset=utf-8";
        if (path.rfind(".js") != std::string::npos) return "application/javascript; charset=utf-8";
        if (path.rfind(".json") != std::string::npos) return "application/json; charset=utf-8";
        if (path.rfind(".svg") != std::string::npos) return "image/svg+xml";
        if (path.rfind(".txt") != std::string::npos) return "text/plain; charset=utf-8";
        return "application/octet-stream";
    }

    bool serve_file(const std::string& filepath, Response& res) {
        std::ifstream file(filepath.c_str(), std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        std::ostringstream ss;
        ss << file.rdbuf();
        res.status = 200;
        res.body = ss.str();
        res.content_type = get_mime_type(filepath);
        return true;
    }

public:
    Server() = default;

    void Get(const std::string& pattern, Handler handler) {
        get_routes_[pattern] = handler;
    }

    void Post(const std::string& pattern, Handler handler) {
        post_routes_[pattern] = handler;
    }

    bool set_mount_point(const std::string& prefix, const std::string& dir) {
        mount_prefix_ = prefix;
        mount_dir_ = dir;
        return true;
    }

    bool listen(const char* host, int port) {
#ifdef _WIN32
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            std::cerr << "[Error] Failed to initialize Winsock." << std::endl;
            return false;
        }
#endif

        socket_t server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (!IS_VALIDSOCKET(server_sock)) {
            std::cerr << "[Error] Could not create socket." << std::endl;
            return false;
        }

        int opt = 1;
#ifdef _WIN32
        setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
        setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

        sockaddr_in server_addr;
        std::memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(static_cast<uint16_t>(port));

        if (std::strcmp(host, "0.0.0.0") == 0) {
            server_addr.sin_addr.s_addr = INADDR_ANY;
        } else {
            server_addr.sin_addr.s_addr = inet_addr(host);
        }

        if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
            std::cerr << "[Error] Failed to bind to " << host << ":" << port << std::endl;
            CLOSESOCKET(server_sock);
            return false;
        }

        if (::listen(server_sock, 10) != 0) {
            std::cerr << "[Error] Failed to listen on socket." << std::endl;
            CLOSESOCKET(server_sock);
            return false;
        }

        is_running_ = true;
        std::cout << "==========================================================" << std::endl;
        std::cout << "  Smart Genome & DNA Sequence Matching for Biodiversity   " << std::endl;
        std::cout << "  BCA 3rd Sem - Algorithm and Problem Solving            " << std::endl;
        std::cout << "==========================================================" << std::endl;
        std::cout << " Server is running at: http://localhost:" << port << "/" << std::endl;
        std::cout << " Open your browser to access the interactive web portal." << std::endl;
        std::cout << " Press Ctrl+C in this terminal to stop the server." << std::endl;
        std::cout << "==========================================================" << std::endl;

        while (is_running_) {
            sockaddr_in client_addr;
#ifdef _WIN32
            int client_len = sizeof(client_addr);
#else
            socklen_t client_len = sizeof(client_addr);
#endif
            socket_t client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
            if (!IS_VALIDSOCKET(client_sock)) {
                continue;
            }

            // Read HTTP request header and body
            std::string raw_request = "";
            char buffer[4096];
            int bytes_read = 0;
            size_t header_end_pos = std::string::npos;
            size_t content_length = 0;

            while ((bytes_read = recv(client_sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
                buffer[bytes_read] = '\0';
                raw_request.append(buffer, bytes_read);

                if (header_end_pos == std::string::npos) {
                    header_end_pos = raw_request.find("\r\n\r\n");
                    if (header_end_pos != std::string::npos) {
                        // Look for Content-Length
                        std::string headers_part = raw_request.substr(0, header_end_pos);
                        size_t cl_pos = headers_part.find("Content-Length:");
                        if (cl_pos == std::string::npos) {
                            cl_pos = headers_part.find("content-length:");
                        }
                        if (cl_pos != std::string::npos) {
                            size_t val_start = headers_part.find_first_not_of(" \t", cl_pos + 15);
                            size_t val_end = headers_part.find("\r\n", val_start);
                            content_length = static_cast<size_t>(std::atol(headers_part.substr(val_start, val_end - val_start).c_str()));
                        }
                    }
                }

                if (header_end_pos != std::string::npos) {
                    size_t body_received = raw_request.length() - (header_end_pos + 4);
                    if (body_received >= content_length) {
                        break; // Request fully received
                    }
                }
            }

            if (raw_request.empty()) {
                CLOSESOCKET(client_sock);
                continue;
            }

            // Parse request line
            Request req;
            std::istringstream req_stream(raw_request);
            std::string req_line;
            if (std::getline(req_stream, req_line)) {
                // Strip \r if present
                if (!req_line.empty() && req_line.back() == '\r') {
                    req_line.pop_back();
                }
                std::istringstream line_stream(req_line);
                line_stream >> req.method >> req.path;
            }

            // Parse headers
            std::string h_line;
            while (std::getline(req_stream, h_line) && h_line != "\r" && !h_line.empty()) {
                if (h_line.back() == '\r') h_line.pop_back();
                size_t colon = h_line.find(':');
                if (colon != std::string::npos) {
                    std::string key = h_line.substr(0, colon);
                    std::string val = h_line.substr(colon + 1);
                    while (!val.empty() && (val.front() == ' ' || val.front() == '\t')) val.erase(val.begin());
                    req.headers[key] = val;
                }
            }

            // Extract body
            if (header_end_pos != std::string::npos) {
                req.body = raw_request.substr(header_end_pos + 4);
            }

            // Prepare response with CORS headers enabled
            Response res;
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
            res.set_header("Access-Control-Allow-Headers", "Content-Type");

            // Handle preflight OPTIONS request
            if (req.method == "OPTIONS") {
                res.status = 200;
                res.set_content("", "text/plain");
            }
            // Handle POST routes
            else if (req.method == "POST" && post_routes_.find(req.path) != post_routes_.end()) {
                post_routes_[req.path](req, res);
            }
            // Handle GET routes
            else if (req.method == "GET" && get_routes_.find(req.path) != get_routes_.end()) {
                get_routes_[req.path](req, res);
            }
            // Handle Static Frontend files
            else if (req.method == "GET") {
                std::string target_path = req.path;
                if (target_path == "/" || target_path.empty()) {
                    target_path = "/index.html";
                }

                // Prevent directory traversal
                if (target_path.find("..") == std::string::npos) {
                    std::string local_file = mount_dir_ + target_path;
                    if (!serve_file(local_file, res)) {
                        res.status = 404;
                        res.set_content("{\"error\":\"404 Not Found: " + target_path + "\"}", "application/json");
                    }
                } else {
                    res.status = 403;
                    res.set_content("{\"error\":\"Forbidden\"}", "application/json");
                }
            } else {
                res.status = 404;
                res.set_content("{\"error\":\"Endpoint not found\"}", "application/json");
            }

            // Send HTTP response
            std::string status_msg = (res.status == 200) ? "OK" : ((res.status == 404) ? "Not Found" : "Error");
            std::ostringstream resp_stream;
            resp_stream << "HTTP/1.1 " << res.status << " " << status_msg << "\r\n";
            resp_stream << "Content-Type: " << res.content_type << "\r\n";
            resp_stream << "Content-Length: " << res.body.length() << "\r\n";
            for (const auto& h : res.headers) {
                resp_stream << h.first << ": " << h.second << "\r\n";
            }
            resp_stream << "Connection: close\r\n\r\n";
            resp_stream << res.body;

            std::string full_response = resp_stream.str();
            send(client_sock, full_response.c_str(), static_cast<int>(full_response.length()), 0);
            CLOSESOCKET(client_sock);
        }

        CLOSESOCKET(server_sock);
#ifdef _WIN32
        WSACleanup();
#endif
        return true;
    }
};

} // namespace httplib
