#pragma once

#include <ranges>


namespace utils
{
    template< std::ranges::range Container, class ValueType = std::ranges::range_value_t< Container > >
    constexpr auto FindOrNullptr(Container&& c, const ValueType& v)
    {
        auto it = std::ranges::find(c, v);
        return it == c.end() ? nullptr : std::addressof(*it);
    }

    template< std::ranges::sized_range Container, class ValueType = std::ranges::range_value_t< Container > >
    constexpr auto ValueAtOrNullptr(Container&& c, std::ranges::range_size_t< Container > index)
    {
        return index < c.size() && index >= 0 ? &c[index] : nullptr;
    }

    template< class Difference, std::ranges::range Container, class Func >
    constexpr auto IndexOrNullptr(Container&& c, Func&& visitor)
    {
        Difference index { 0 };
        while (index < static_cast< Difference >(std::ranges::size(c)))
        {
            if (visitor(c[index])) return index;
            ++index;
        }

        return -1;
    }

    template< std::ranges::range Container, class Func >
    constexpr auto IndexOrNullptr(Container&& c, Func&& visitor)
    {
        return IndexOrNullptr< std::ranges::range_difference_t< Container > >(std::forward< Container >(c), std::forward< Func >(visitor));
    }
}
