g++ -std=c++17 src/main.cpp src/core/thread_pool.cpp src/http/response.cpp src/handlers/static_handler.cpp src/handlers/proxy_handler.cpp -o server -pthread
