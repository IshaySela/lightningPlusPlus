# Lightning++

  

⚡**Lightning++** is a high-performance, lightweight HTTPS backend framework built with modern C++20 .
🚀 Designed with a focus on speed, security, and developer productivity, it provides a clean, expressive API for building robust web services while maintaining the granular control over system resources that C++ developers expect.

##  Performance and Scalability
At its core, **lightning++** is engineered for high-concurrency environments. The framework utilizes a **Thread Pool architecture** (via the `TaskExecutor` class) to handle incoming requests concurrently. 

## 💻 Quick Start

```cpp
#include <lightning/httpServer/ServerBuilder.hpp>

auto main() -> int {
    // Configure & build the server
    auto server = lightning::ServerBuilder::createNew(8080)
                      .withSsl("cert.pem", "key.pem")
                      .withThreads(8)
                      .build();

    // Define endpoints
    server.get("/hello", [](lightning::HttpRequest request) {
        return lightning::HttpResponseBuilder::create()
            .withBody("Hello, World!")
            .build();
    });
    // Server static folder
    server.get("/*", lightning::handlers::serveFolder("./public"));
    
	// Run the server
    server.start();
    return 0;
}
```

## 🏗️ Architecture & Request Lifecycle

The project follows a modular, interface-driven design that strictly separates protocol logic from underlying transport mechanisms.

### Request Lifecycle
```mermaid
graph LR
    A[Accept Client] --> B[Parse Request &<br>Map to Endpoint]
    B --> C[Push Task to Thread<br>Pool]
    C --> D[Run Endpoint]
    D --> E[Write response]
    C --> |Concurrent| A
```

When a client connection is accepted, the request is parsed and immediately dispatched to a worker thread. This decoupled nature ensures that the main acceptance loop remains unblocked, allowing the server to scale linearly with the number of available CPU cores. By leveraging **C++20** features and optimized string processing, the framework minimizes overhead during request routing and header parsing, making it an ideal choice for latency-sensitive applications. 

 
## 🗺️ Roadmap

The project is actively evolving with the following priorities:
-   **Non-blocking I/O Integration:** Migrating the core event loop to utilize  high-performance APIs like `epoll` or `io_uring` to handle thousands of simultaneous connections with minimal overhead.
- **C++20 Coroutine Support:** Integrating `std::coroutine` for a seamless async/await developer experience.
-   **Python Bindings:** Developing a C++ extension module to allow Python developers to leverage the framework's performance while writing handlers in a high-level language.
-   **Increased Observability:** Adding built-in support for structured logging, performance metrics (request latency, throughput), and health check endpoints. 
