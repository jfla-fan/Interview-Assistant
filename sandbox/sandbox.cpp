#include "utils/numeric.h"

#include <QList>
#include <QDebug>

#include <inja/environment.hpp>


static inja::json GetJsonPath(const inja::json& j, const std::string& path) {
    std::istringstream iss(path);
    std::string token;
    auto current = j;

    while (std::getline(iss, token, '.')) {
        if (token.empty()) continue;

        if (current.is_object() && current.contains(token)) {
            current = current[token];
        } else if (current.is_array()) {
            try {
                size_t index = std::stoul(token);
                if (index < current.size()) {
                    current = current[index];
                } else {
                    return nullptr; // out of bounds
                }
            } catch (...) {
                return nullptr; // invalid index
            }
        } else {
            return nullptr; // key not found
        }
    }

    return current;
}

int main(int argc, char* argv[])
{
    
#ifdef BUILD_SANDBOX
    qDebug() << "Build sandbox enabled2";
#endif

    std::string line = R"({ "value": "Line!\n\r" })";

    QString line2 = QString::fromStdString(line);
    QString line3 = "Line!\n\r\"";

    inja::Environment env;

    using object_t = inja::json::object_t;
    using string_t = inja::json::string_t;

    inja::json data2 = {
        { "data1", object_t {
                      { "name1", "value\n\r1\t" },
                      { "name2", object_t { { "name3", "value\n\r\\t" } } }
                   }
        },
        { "nonparsed", R"({ "value": "Line!\n\r" })" },
        { "number", 4 },
        { "boolean", true },
        { "string", "some \bdata \rhere\n" },
        { "empty", {} },
        { "Type", "image/png" },
        { "ImageData", "daysyrew1231" }
    };

    env.add_callback("get", 2, [](inja::Arguments& args) {
        const inja::json& obj = args[0]->get< inja::json >();
        std::string path = args[1]->get< std::string >();
        return GetJsonPath(obj, path);
    });

    env.add_callback("json_parse", [](inja::Arguments& args) {
        return inja::json::parse(args.at(0)->get< std::string >());
    });

    env.add_callback("json_extract", [](inja::Arguments& args) {
        const auto& src = args.at(0);
        std::string path = args.at(1)->get< std::string >();

        inja::json parsed;

        // non-parsed
        if (src->is_string()) {
            try {
                parsed = inja::json::parse(src->get<std::string>());
            } catch (const inja::json::parse_error& e) {
                qDebug() << "JSON parse error in json_extract:" << e.what();
                return inja::json(nullptr); // or return default value
            }
        } else {
            // Already a json object/array
            parsed = *src;
        }

        return GetJsonPath(parsed, path);
    });

    env.add_callback("json_escape", 1, [](inja::Arguments& args) {
        // if (args.empty()) return inja::json("");

        // const auto& arg = args.front();

        // if (arg->is_object() || arg->is_array() ||
        //     arg->is_number() || arg->is_boolean()) {
        //     return inja::json{ arg->dump() };
        // }
        // throw std::runtime_error("JSON ESCAPE RUNTIME ERROR");
        // return inja::json { inja::json(arg->get<std::string>()).dump() };
        return args.front()->dump();
    });

    env.add_callback("getexample", 0, [&data2](inja::Arguments& args) {
        return data2;
    });

    env.add_callback("concat", [](inja::Arguments& args) {
        inja::json::string_t result;

        for (const auto* arg : args) {
            if (arg->is_string()) {
                result += arg->get_ref< const inja::json::string_t& >();
            }
            else {
                result += arg->dump();
            }
        }

        return result;
    });

    try {
        auto rendered = env.render(R"({{ concat("data:", Type, ";base64,", ImageData) | json_escape }})", data2);

        qDebug() << "Rendered: " << rendered;
    } catch (const std::exception& ex) {
        qDebug() << "Got exception: " << ex.what();
    }

    return 0;
}
