#pragma once

#include <optional>
#include <string_view>
#include <limits>

#include <QDebug>


namespace utils
{
    template< class To, class From > requires(std::is_integral_v< To > && std::is_integral_v< From >)
    std::optional< To > safe_cast(From from)
    {
        using FromLimits = std::numeric_limits< From >;
        using ToLimits   = std::numeric_limits< To >;

        std::string_view overflow_type;

        if constexpr (ToLimits::digits < FromLimits::digits) {
            if (from > static_cast< From >(ToLimits::max())) {
                overflow_type = "positive";
            }
        }

        // signed -> signed: loss if narrowing
        // signed -> unsigned: loss
        if constexpr (FromLimits::is_signed && (!ToLimits::is_signed || ToLimits::digits < FromLimits::digits)) {
            if (from < static_cast< From >(ToLimits::lowest())) {
                overflow_type = "negative";
            }
        }

        if (!overflow_type.empty()) {
            qWarning() << "Failed to convert " << typeid(From).name() << " into " << typeid(To).name() << ". Overflow: " << overflow_type;
            return {};
        }

        return static_cast< To >(from);
    }
}
