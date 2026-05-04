#pragma once

#include "service_config.h"


class ServiceConfig::ConfigPrivate {
public:
    QString m_path;
    QString m_originalText;
    ParserPtr m_parser;
    RendererPtr m_renderer;
    Optional< Root > m_root;
    QList< ServiceProxy > m_renderedProxies;
    QList< ServiceRequestLookUpData > m_requestLookUpData;
};
