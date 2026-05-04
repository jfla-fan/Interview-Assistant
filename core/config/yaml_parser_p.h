#pragma once

#include "yaml_parser.h"

#include <yaml-cpp/yaml.h>


class ServiceConfig::YamlParserPrivate {
public:
    QString m_strConfig;
    YAML::Node m_parsedConfig;

    CStorage< YAML::Node > parseTemplates();
    YAML::Node expandNode(const YAML::Node& node, const CStorage< YAML::Node >& templates) const;
    YAML::Node mergeNodes(const YAML::Node& base, const YAML::Node& overrides, const CStorage< YAML::Node >& templates) const;
    QString nodeToString(const YAML::Node& node) const;
};
