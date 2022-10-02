// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_NUMBERUTILS_H
#define INCLUDED_NUMBERUTILS_H

#if defined(_MSC_VER)
#define really_inline __forceinline
#else
#define really_inline inline __attribute__((always_inline))
#endif

#include <cstdlib>
#include <locale>
#include <system_error>
#include <type_traits>

namespace OCIO_NAMESPACE
{
namespace NumberUtils
{

struct Locale
{
#ifdef _WIN32
    Locale() : local(_create_locale(LC_ALL, "C"))
    {
    }
    ~Locale()
    {
        _free_locale(local);
    }
    _locale_t local;
#else
    Locale() : local(newlocale(LC_ALL_MASK, "C", NULL))
    {
    }
    ~Locale()
    {
        freelocale(local);
    }
    locale_t local;
#endif
};

struct from_chars_result
{
    const char *ptr;
    std::errc ec;
};

static const Locale loc;

really_inline from_chars_result from_chars(const char *first, const char *last, double &value) noexcept
{
    errno = 0;
    if (!first || !last || first == last)
    {
        return {first, std::errc::invalid_argument};
    }

    char * endptr = nullptr;

    double
#ifdef _WIN32
    tempval = _strtod_l(first, &endptr, loc.local);
#else
    tempval = ::strtod_l(first, &endptr, loc.local);
#endif

    if (errno != 0 && errno != EINVAL)
    {
        return {first + (endptr - first), std::errc::result_out_of_range};
    }
    else if (endptr == first)
    {
        return {first, std::errc::invalid_argument};
    }
    else if (endptr <= last)
    {
        value = tempval;
        return {first + (endptr - first), {}};
    }
    else
    {
        return {first, std::errc::argument_out_of_domain};
    }
}

really_inline from_chars_result from_chars(const char *first, const char *last, float &value) noexcept
{
    errno = 0;
    if (!first || !last || first == last)
    {
        return {first, std::errc::invalid_argument};
    }

    char *endptr = nullptr;

    float
#ifdef _WIN32
#if defined(__MINGW32__) || defined(__MINGW64__)
    // MinGW doesn't define strtof_l (clang/gcc) nor strtod_l (gcc)...
    tempval = static_cast<float>(_strtod_l (first, &endptr, loc.local));
#else
    tempval = _strtof_l(first, &endptr, loc.local);
#endif
#elif __APPLE__
    // On OSX, strtod_l is for some reason drastically faster than strtof_l.
    tempval = static_cast<float>(::strtod_l(first, &endptr, loc.local));
#else
    tempval = ::strtof_l(first, &endptr, loc.local);
#endif

    if (errno != 0)
    {
        return {first + (endptr - first), std::errc::result_out_of_range};
    }
    else if (endptr == first)
    {
        return {first, std::errc::invalid_argument};
    }
    else if (endptr <= last)
    {
        value = tempval;
        return {first + (endptr - first), {}};
    }
    else
    {
        return {first, std::errc::argument_out_of_domain};
    }
}

really_inline from_chars_result from_chars(const char *first, const char *last, long int &value) noexcept
{
    errno = 0;
    if (!first || !last || first == last)
    {
        return {first, std::errc::invalid_argument};
    }

    char *endptr = nullptr;

    long int
#ifdef _WIN32
    tempval = _strtol_l(first, &endptr, 0, loc.local);
#elif defined(__GLIBC__)
    tempval = ::strtol_l(first, &endptr, 0, loc.local);
#else
    tempval = ::strtol(first, &endptr, 0);
#endif

    if (errno != 0)
    {
        return {first + (endptr - first), std::errc::result_out_of_range};
    }
    else if (endptr == first)
    {
        return {first, std::errc::invalid_argument};
    }
    else if (endptr <= last)
    {
        value = tempval;
        return {first + (endptr - first), {}};
    }
    else
    {
        return {first, std::errc::argument_out_of_domain};
    }
}
} // namespace NumberUtils
} // namespace OCIO_NAMESPACE
#endif // INCLUDED_NUMBERUTILS_H
