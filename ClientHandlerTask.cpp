#include "lightning/httpServer/ClientHandlerTask.hpp"


namespace lightning
{
    ClientHandlerTask::ClientHandlerTask(std::unique_ptr<IClient> client,
        Resolver resolver,
        HttpRequest request,
        MiddlewareContainer<>* middlewareChains) : client(std::move(client)),
        resolver(resolver),
        request(request),
        middlewares(middlewareChains)
    {
    }
    auto ClientHandlerTask::operator()() -> void
    {
        // This is needs to be done on the working thread
        // And can only be done after frameworkInfo was injected.
        request.computeUriParameters();
        std::optional<HttpResponse> preMiddlewareResult;

        for (auto& currPreMiddleware : this->middlewares->getPreMiddlewares())
        {
            preMiddlewareResult = currPreMiddleware(request);

            
            if (preMiddlewareResult.has_value())
                break;

        }

        // If one of the pre middleware has returned a response, 
        // ues that response instead.
        auto response = preMiddlewareResult.has_value() ? preMiddlewareResult.value() : this->resolver(request);

        for (auto& post : this->middlewares->getPostMiddlewares())
        {
            if (!post(response)) {
                break;
            }
        }

        auto responseBuffer = response.toHttpResponse();
        this->client->getStream().write(responseBuffer.data(), responseBuffer.size());
    }
} // namespace lightning
