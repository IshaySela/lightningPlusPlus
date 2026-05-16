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
    Client1[Client] -->|request| A
    Client2[Client] -->|request| A
    Client3[Client] -->|request| A
    Client4[...] -->|request| A

    A[Accept Client] --> B[Push Task to<br>Thread Pool]
    B --> |Concurrent| A
    B --> C[Parse Request &<br>Resolve Endpoint]
    C --> D[Run Handler]
    D --> E[Write Response]
```

When a client connection is accepted, it is immediately pushed to the thread pool — parsing and endpoint resolution happen entirely inside the worker thread. This keeps the main acceptance loop as tight as possible, so it can pick up the next connection without waiting for any request processing. By moving all per-request work (header parsing, URI matching, handler dispatch) off the hot path, the server scales linearly with the number of available CPU cores and remains an ideal choice for latency-sensitive, high-concurrency applications.

 
## 🗺️ Roadmap

The project is actively evolving with the following priorities:
-   **Non-blocking I/O Integration:** Migrating the core event loop to utilize  high-performance APIs like `epoll` or `io_uring` to handle thousands of simultaneous connections with minimal overhead.
- **C++20 Coroutine Support:** Integrating `std::coroutine` for a seamless async/await developer experience.
-   **Python Bindings:** Developing a C++ extension module to allow Python developers to leverage the framework's performance while writing handlers in a high-level language.
-   **Increased Observability:** Adding built-in support for structured logging, performance metrics (request latency, throughput), and health check endpoints. 
