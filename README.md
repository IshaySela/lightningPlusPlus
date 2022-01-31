# lightningPlusPlus
Lightning fast backend framework written in c++


```cpp

auto main() -> void
{
  lightning::SSLServer sslServer(8080, CERT_FILE_PATH, PRIVATE_KEY_PATH);

  lightning::HttpServer httpServer(sslServer);

  httpServer.get("/*", lightning::handlers::serveFolder("../tests"));
  httpServer.start();
}
```
