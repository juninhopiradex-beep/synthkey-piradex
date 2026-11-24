#pragma once
#include "Core/Licensing.h"

namespace pwtab
{
inline constexpr sk::lic::PwEntry kPasswords[] =
{
    { "master", sk::lic::Role::Master,
      "87411841d831f3f43e17f9e764c78692",
      "afee6bb20b23040349faad43e667e120b5a6404e0c69b2b6142dbd38d8e5de10", 0, 0, 0 },

    { "beta-01", sk::lic::Role::Beta,
      "4c636d68a959c0c8194bd7c05a15bac7",
      "1bc9c976dbbcbfabe71d9c541a08cd52854143641581a0fc8c927e4f15a94d74", 2026, 12, 31 },
};
inline constexpr int kPasswordCount = (int) (sizeof (kPasswords) / sizeof (kPasswords[0]));
} // namespace pwtab
