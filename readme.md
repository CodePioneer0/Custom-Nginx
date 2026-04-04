# 🚀 Mini Nginx (C++ HTTP Server)

## 🎯 Final Goal
A high-performance HTTP server that:
- Handles thousands of concurrent connections
- Uses **epoll** (async I/O)
- Supports static file serving
- Uses a thread pool
- Implements basic load balancing

## 🧱 1. High-Level Architecture

```text
       Client (Browser / curl)
                 ↓
             TCP Socket
                 ↓
         Event Loop (epoll)
                 ↓
           Request Parser
                 ↓
          Router / Handler
                 ↓
          Response Builder
                 ↓
   File System / Upstream Servers
```

## ⚙️ 2. Tech Stack
- **Core:** C++17/20, Linux sockets, `epoll` (VERY IMPORTANT)
- **Optional but strong:** Boost.Asio (for abstraction), `sendfile()` (for zero-copy fast file transfer)

## 📦 3. Folder Structure
```text
mini-nginx/
│
├── src/
│   ├── main.cpp
│   ├── server/
│   │   ├── server.cpp
│   │   ├── connection.cpp
│   │   ├── epoll_manager.cpp
│   │
│   ├── http/
│   │   ├── request.cpp
│   │   ├── response.cpp
│   │   ├── parser.cpp
│   │
│   ├── core/
│   │   ├── thread_pool.cpp
│   │   ├── logger.cpp
│   │
│   ├── handlers/
│   │   ├── static_handler.cpp
│   │   ├── proxy_handler.cpp
│   │
│   └── utils/
│       ├── config.cpp
│
├── include/
├── config/
│   └── server.conf
└── www/
    └── index.html
```

## 🔥 4. Step-by-Step Build Plan

### 🟢 PHASE 1: Basic TCP Server
**Goals:** Create socket, Bind + listen, Accept multiple clients.
**Key APIs:** `socket()`, `bind()`, `listen()`, `accept()`, `recv()`, `send()`
👉 **Output:** Server responds: "Hello from server"

### 🟡 PHASE 2: HTTP Parsing
**Goals:** Understand and parse HTTP structure.
```http
GET /index.html HTTP/1.1
Host: localhost:8080
```
**Build:** Request parser (Method: GET/POST, Path: /index.html, Headers).
👉 **Output:** Print parsed request

### 🟠 PHASE 3: HTTP Response
**Goals:** Build proper HTTP response formatting.
```http
HTTP/1.1 200 OK
Content-Type: text/html
Content-Length: 13

Hello, World!
```
👉 **Implement:** Status codes, Headers, Body

### 🔵 PHASE 4: Static File Server
**Flow:** Request `/index.html` → Map to `./www/index.html` → Read file → Send response.
👉 **Optimize:** Use `sendfile()` (huge plus in interviews for zero-copy transfers).

### 🔴 PHASE 5: epoll (ASYNC CORE)
**Goals:** Replace blocking I/O with `epoll` to handle 10k+ connections efficiently.
**Key APIs:** `epoll_create1()`, `epoll_ctl()`, `epoll_wait()`
**Design:** Non-blocking sockets, Event-driven loop.
👉 **Note:** This is the absolute heart of the project.

### 🟣 PHASE 6: Thread Pool
**Why?** `epoll` handles I/O, threads handle processing.
**Design:** Task queue, Worker threads.
**Flow:** `epoll` → push task → thread pool → process request.

### 🟤 PHASE 7: Routing System
**Goals:** Create handler mappings.
- `/` → `index.html`
- `/api` → custom handler
- `/static` → file server

### ⚫ PHASE 8: Reverse Proxy + Load Balancer
**Concept:** Forward requests to backend servers (`Client → Your Server → Backend1 / Backend2`).
**Load Balancing Algorithms:**
- Round Robin ✅ (implement this)
- Least connections (optional)
**Flow:** Receive request → Pick backend server → Forward request → Get response → Send back to client.

### ⚪ PHASE 9: Configuration File
**Goals:** Parse config dynamically (like Nginx).
```ini
port=8080
root=./www
threads=4

upstream backend {
    server 127.0.0.1:9001;
    server 127.0.0.1:9002;
}
```

### 🧠 PHASE 10: Logging + Metrics
- Access logs
- Error logs
- Request timing

## ⚡ 5. Advanced Features (For Top-Tier Impact)
- Keep-Alive connections
- HTTP/1.1 pipelining
- Rate limiting
- Caching
- Gzip compression

## 🧪 6. Testing
Make sure to test concurrently:
```bash
curl http://localhost:8080
ab -n 10000 -c 100 http://localhost:8080/
wrk http://localhost:8080/
```

## 📈 7. Deployment
- Run on AWS / VPS
- Use Nginx as a benchmark comparison
- Show metrics: Requests/sec, Latency

## 🧾 8. Resume Impact Line
> "Built a high-performance HTTP server in C++ using epoll-based async I/O, handling 10,000+ concurrent connections with custom thread pool and reverse proxy load balancing."

---
_Don’t try to build everything at once. strictly follow the phase-by-phase build plan._
