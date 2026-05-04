#pragma once

#include "service_config.h"


namespace ServiceConfig
{
    class InjaRendererPrivate;

    class InjaRenderer : public ITemplateEngine
    {
    public:
        InjaRenderer();
        ~InjaRenderer() override;

        bool initialize() override;

        Optional< CString > renderWithGlobalVars(const CString& document, const GlobalVars& vars) const override;
        Optional< CString > renderWithJsonVars(const CString& document, const CString& jsonData, const GlobalVars& vars) const override;
        Optional< CString > renderWithJsonVars(const CString& document, const QJsonObject& vars) const;

        QVariant parse(const CString& document) const override;
        RendererPtr clone() const override;

    private:
        std::unique_ptr< InjaRendererPrivate > d_ptr;
        Q_DECLARE_PRIVATE(InjaRenderer);
    };
}
