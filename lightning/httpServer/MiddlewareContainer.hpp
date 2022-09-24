#pragma once
#include <functional>
#include "../request/HttpRequest.hpp"
#include "../response/HttpResponse.hpp"

namespace lightning
{

    // A middleware must have an operator () that takes a lightning::HttpRequest& and returns
    // Something that can be converted to boolean.
    template<typename T>
    concept PreMiddleware = requires(T m, lightning::HttpRequest & request)
    {
        {m.operator()(request)} -> std::convertible_to<bool>;
    }&& std::copy_constructible<T>;

    template<typename T>
    concept PostMiddleware = requires(T m, lightning::HttpResponse & response)
    {
        { m.operator()(response) } -> std::convertible_to<bool>;
    }&& std::copy_constructible<T>;


    template<typename TPre = std::function<bool(lightning::HttpRequest&)>, typename TPost = std::function<bool(lightning::HttpResponse&)>> requires PreMiddleware<TPre>&& PostMiddleware<TPost>
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