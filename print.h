#pragma once

#include <iostream>
#include <string_view>
#include <format>
#include <utility>

template <typename ...ARGS>
void print(std::string_view format, ARGS    &&...args)
{
    std::cout << std::format(format,std::forward<ARGS>(args)...);
}