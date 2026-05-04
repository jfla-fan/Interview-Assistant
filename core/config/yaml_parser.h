#pragma once

#include "service_config.h"

#include <yaml-cpp/yaml.h>


namespace ServiceConfig
{

class YamlParserPrivate;

class YamlParser final : public IConfigParser
{
public:
    YamlParser();
    ~YamlParser() override;

    bool load(QString input) override;
    bool preprocess() override;
    Optional< Root > dump() override;
    QVariant parse(const QString& input) const override;

    const QString& getConfig() const override;

    void printParsedConfigDebug(const QVariant& value, int indent = 0);
    YAML::Node getConfigDebug() const;
    QString nodeToStringDebug(const YAML::Node& node) const;
    ParserPtr clone() const override;
private:
    std::unique_ptr< YamlParserPrivate > d_ptr;
    Q_DECLARE_PRIVATE(YamlParser)
};

}
