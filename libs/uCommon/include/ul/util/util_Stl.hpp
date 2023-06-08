
#pragma once
#include <algorithm>

#define UL_STL_FIND_IF(stl_item, var_name, cond) std::find_if(stl_item.begin(), stl_item.end(), [&](const auto &var_name){ return (cond); })
#define UL_STL_FOUND(stl_item, find_ret) (find_ret != stl_item.end())
#define UL_STL_UNWRAP(find_ret) (*find_ret)
#define UL_STL_REMOVE_IF(stl_item, var_name, cond) stl_item.erase(std::remove_if(stl_item.begin(), stl_item.end(), [&](const auto &var_name){ return (cond); }), stl_item.end())
