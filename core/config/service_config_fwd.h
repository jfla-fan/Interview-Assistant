#pragma once

#include <memory>


namespace ServiceConfig
{
    struct BaseNode;
    struct GlobalVar;
    struct Proxy;
    struct Request;
    struct Reply;
    struct Display;
    struct HttpRequest;
    struct Service;
    struct ServiceProvider;
    struct Root;

    class IConfigParser;
    using ParserPtr = std::shared_ptr< IConfigParser >;

    class ITemplateEngine;
    using RendererPtr = std::shared_ptr< ITemplateEngine >;

    struct RenderingContext;

    class Config;

    using ConfigPtr = std::shared_ptr< Config >;
    using ConstConfigPtr = std::shared_ptr< const Config >;

    class ConfigLoader;
}
