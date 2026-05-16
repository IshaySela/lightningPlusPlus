#pragma once
#include <functional>
#include "../lightning.hpp"
#include "../request/HttpRequest.hpp"
#include "../response/HttpResponse.hpp"

namespace lightning
{

    template<typename T>
    concept PreMiddleware = requires(T m, lightning::HttpRequest & request)
    {
        {m.operator()(request)} -> std::convertible_to<std::optional<HttpResponse>>;
    }&& std::copy_constructible<T>;

    template<typename T>
    concept PostMiddleware = requires(T m, lightning::HttpResponse & response)
    {
        { m.operator()(response) } -> std::convertible_to<bool>;
    }&& std::copy_constructible<T>;

    template<typename TPre = DefaultPreMiddlewareType, typename TPost = DefaultPostMiddlewareType> requires PreMiddleware<TPre> && PostMiddleware<TPost>
    class MiddlewareContainer
    {
    public:
        auto addPost(TPost postMiddleware) -> void
        {
            this->postMiddlewares.push_back(postMiddleware);
        };

        auto addPre(TPre preMiddleware) -> void
        {
            this->preMiddlewares.push_back(preMiddleware);
        };

        auto getPreMiddlewares()->std::vector<TPre>&
        {
            return this->preMiddlewares;
        }
        auto getPostMiddlewares()->std::vector<TPost>&
        {
            return this->postMiddlewares;
        }
    private:
        std::vector<TPre> preMiddlewares;
        std::vector<TPost> postMiddlewares;
    };
}