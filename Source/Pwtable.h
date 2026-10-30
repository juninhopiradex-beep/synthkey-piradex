#pragma once
#include "Core/Licensing.h"

namespace pwtab
{
inline constexpr sk::lic::PwEntry kPasswords[] =
{
    { "master", sk::lic::Role::Master,
      "bc97bf3b48b154028ef2deaf26921eed",
      "5be2fe5449880db49380e2f8451f3c85108fba6c7598fa8184a7d8640810878d", 0, 0, 0 },

    { "beta-01", sk::lic::Role::Beta,
      "ba3dd9325c2859b7c5e0775ca03913ab",
      "7e8152cf63c17fe8c341885926b99b2f8d77b827bf2b013e58cff7c232f793ca", 2026, 12, 31 },
    { "beta-02", sk::lic::Role::Beta,
      "4239640131bd793b1a520c889787b7dd",
      "5a4290c0cd300c32ae6e337970c7d7198f5b9d53ea78e23ece75ec21282db504", 2026, 12, 31 },
    { "beta-03", sk::lic::Role::Beta,
      "f233dae5df0c87fa3971612d007b1995",
      "9c25fba503e62b3a8260497a033fe0a5bb1781fe335f1f46f23e81b906c6286d", 2026, 12, 31 },
};
inline constexpr int kPasswordCount = (int) (sizeof (kPasswords) / sizeof (kPasswords[0]));
} // namespace pwtab
