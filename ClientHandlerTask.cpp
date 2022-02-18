#include "lightning/httpServer/ClientHandlerTask.hpp"


namespace lightning
{
    ClientHandlerTask::ClientHandlerTask(std::unique_ptr<IClient> client, Resolver resolver, HttpRequest request) : client(std::move(client)), resolver(resolver), request(request)
    {
    }
    auto ClientHandlerTask::operator()() -> void
    {
        // Here all of the middlewares (future) and pre resolve tasks execute

        // This is needs to be done on the working thread
        // And can only be done after frameworkInfo was injected.
        request.computeUriParameters();

        auto response = this->resolver(request).toHttpResponse();
        this->client->getStream().write(response.data(), response.size());
    }
} // namespace lightning
