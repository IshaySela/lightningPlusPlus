#include "lightning/httpServer/HttpServer.hpp"
#include "lightning/httpServer/NonblockingClientManagerTask.hpp"
#include <fcntl.h>
#include <thread>
#include <unistd.h>

namespace lightning
{

    const HttpServer::ShouldStopPredicate HttpServer::neverStop = [](HttpServer&)
    { return false; };
    

    const Resolver HttpServer::defaultResolver = [](HttpRequest request) -> lightning::HttpResponse
    {
        return HttpResponseBuilder::create()
            .withStatusCode(404)
            .withStatusPhrase("Not Found")
            .build();
    };

    HttpServer::HttpServer(std::unique_ptr<ILowLevelSocketServer> lowLevelServer, const int threadCount)
        : lowLevelServer(std::move(lowLevelServer)), threadCount(threadCount)
    {
        for (int i = 0; i < HttpProtocol::supportedHttpMethods.size(); i++)
        {
            auto& method = HttpProtocol::supportedHttpMethods[i];

            this->resolvers.insert({ method, UriMapper() });
        }
    }

    auto HttpServer::start(ShouldStopPredicate shouldStop) -> void
    {
        int newFdPipe[2];
        int returnPipe[2];
        pipe2(newFdPipe, O_NONBLOCK | O_CLOEXEC);
        pipe2(returnPipe, O_NONBLOCK | O_CLOEXEC);

        newFdChannel.pipeRead   = newFdPipe[0];
        newFdChannel.pipeWrite  = newFdPipe[1];
        returnChannel.pipeRead  = returnPipe[0];
        returnChannel.pipeWrite = returnPipe[1];

        int tc = this->threadCount;
        std::thread epollThread([this, tc]() {
            NonblockingClientManagerTask manager(newFdChannel, returnChannel, tc, *this);
            manager();
        });

        do
        {
            std::unique_ptr<IClient> client = this->lowLevelServer->accept();
            int fd = client->getFd();
            fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);

            {
                std::lock_guard lock(newFdChannel.m);
                newFdChannel.clients.push_back(std::move(client));
            }
            char byte = 'x';
            ::write(newFdChannel.pipeWrite, &byte, 1);
        } while (!shouldStop(*this));

        ::close(newFdChannel.pipeWrite);
        newFdChannel.pipeWrite = -1;
        epollThread.join();

        ::close(newFdChannel.pipeRead);
        ::close(returnChannel.pipeRead);
        ::close(returnChannel.pipeWrite);
    }

    auto HttpServer::get(std::string uri, Resolver resolver) -> void
    {
        if (uri == "*")
        {
            this->defaultGetResolver = resolver;
        }
        else
        {
            this->addResolver(HttpProtocol::Method::Get, uri, resolver);
        }
    }

    auto HttpServer::post(std::string uri, Resolver resolver) -> void
    {
        this->addResolver(HttpProtocol::Method::Post, uri, resolver);
    }
    auto HttpServer::put(std::string uri, Resolver resolver) -> void
    {
        this->addResolver(HttpProtocol::Method::Put, uri, resolver);
    }
    auto HttpServer::head(std::string uri, Resolver resolver) -> void
    {
        this->addResolver(HttpProtocol::Method::Head, uri, resolver);
    }
    auto HttpServer::resolveDelete(std::string uri, Resolver resolver) -> void
    {
        this->addResolver(HttpProtocol::Method::Delete, uri, resolver);
    }
    auto HttpServer::addResolver(HttpProtocol::Method method, std::string uri, Resolver resolver) -> void
    {
        auto methodString = HttpProtocol::convertMethodToString(method);

        this->resolvers.at(methodString).add(uri, resolver);
    }
    auto HttpServer::getResolver(std::string method, std::string uri, std::string& regexUri) -> std::optional<Resolver>
    {
        auto methodMap = this->resolvers.find(method);

        if (methodMap == this->resolvers.end())
            return std::nullopt;

        auto urisMap = (*methodMap).second;

        auto res = urisMap.match(uri);

        if (res.has_value())
        {
            auto [resolver, rgx] = res.value();

            regexUri = rgx;
            return resolver;
        }

        return std::nullopt;
    }

    auto HttpServer::getResolver(std::string method, std::string uri) -> std::optional<Resolver>
    {
        std::string discard;
        return this->getResolver(method, uri, discard);
    }


    auto HttpServer::getTimeSinceEpoch() -> std::uint64_t
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();
    }

    auto HttpServer::usePreMiddleware(DefaultPreMiddlewareType middleware) -> void
    {
        this->middlewares.addPre(middleware);
    }
    auto HttpServer::usePostMiddleware(DefaultPostMiddlewareType middleware) -> void
    {
        this->middlewares.addPost(middleware);
    }
}