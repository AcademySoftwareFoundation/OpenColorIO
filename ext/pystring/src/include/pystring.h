// Copyright Contributors to the Pystring project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/imageworks/pystring/blob/master/LICENSE


// This is a modified version of the Pystring project, combining .h and .cpp
// files to convert it into a header-only implementation.

#ifndef INCLUDED_PYSTRING_H
#define INCLUDED_PYSTRING_H

#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <iostream>
#include <sstream>


// ####################################################################
// ####################################################################
//                              Declarations
// ####################################################################
// ####################################################################
namespace pystring
{

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @mainpage pystring
    ///
    /// This is a set of functions matching the interface and behaviors of python string methods
    /// (as of python 2.3) using std::string.
    ///
    /// Overlapping functionality ( such as index and slice/substr ) of std::string is included
    /// to match python interfaces.
    ///

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @defgroup functions pystring
    /// @{


    constexpr const int MAX_32BIT_INT = 2147483647;

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Return a copy of the string with only its first character capitalized.
    ///
    std::string capitalize( const std::string & str );

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Return centered in a string of length width. Padding is done using spaces.
    ///
    std::string center( const std::string & str, int width );

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Return the number of occurrences of substring sub in string S[start:end]. Optional
    /// arguments start and end are interpreted as in slice notation.
    ///
    int count( const std::string & str, const std::string & substr, int start = 0, int end = MAX_32BIT_INT);

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Return True if the string ends with the specified suffix, otherwise return False. With
    /// optional start, test beginning at that position. With optional end, stop comparing at that position.
    ///
    bool endswith( const std::string & str, const std::string & suffix, int start = 0, int end = MAX_32BIT_INT );

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Return a copy of the string where all tab characters are expanded using spaces. If tabsize
    /// is not given, a tab size of 8 characters is assumed.
    ///
    std::string expandtabs( const std::string & str, int tabsize = 8);

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Return the lowest index in the string where substring sub is found, such that sub is
    /// contained in the range [start, end). Optional arguments start and end are interpreted as
    /// in slice notation. Return -1 if sub is not found.
    ///
    int find( const std::string & str, const std::string & sub, int start = 0, int end = MAX_32BIT_INT  );

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Synonym of find right now. Python version throws exceptions. This one currently doesn't
    ///
    int index( const std::string & str, const std::string & sub, int start = 0, int end = MAX_32BIT_INT  );

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Return true if all characters in the string are alphanumeric and there is at least one
    /// character, false otherwise.
    ///
    bool isalnum( const std::string & str );

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Return true if all characters in the string are alphabetic and there is at least one
    /// character, false otherwise
    ///
    bool isalpha( const std::string & str );

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Return true if all characters in the string are digits and there is at least one
    /// character, false otherwise.
    ///
    bool isdigit( const std::string & str );

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Return true if all cased characters in the string are lowercase and there is at least one
    /// cased character, false otherwise.
    ///
    bool islower( const std::string & str );

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Return true if there are only whitespace characters in the string and there is at least
    /// one character, false otherwise.
    ///
    bool isspace( const std::string & str );

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Return true if the string is a titlecased string and there is at least one character,
    /// i.e. uppercase characters may only follow uncased characters and lowercase characters only
    /// cased ones. Return false otherwise.
    ///
    bool istitle( const std::string & str );

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Return true if all cased characters in the string are uppercase and there is at least one
    /// cased character, false otherwise.
    ///
    bool isupper( const std::string & str );

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Return a string which is the concatenation of the strings in the sequence seq.
    /// The separator between elements is the str argument
    ///
    std::string join( const std::string & str, const std::vector< std::string > & seq );

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Return the string left justified in a string of length width. Padding is done using
    /// spaces. The original string is returned if width is less than str.size().
    ///
    std::string ljust( const std::string & str, int width );

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Return a copy of the string converted to lowercase.
    ///
    std::string lower( const std::string & str );

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Return a copy of the string with leading characters removed. If chars is omitted or None,
    /// whitespace characters are removed. If given and not "", chars must be a string; the
    /// characters in the string will be stripped from the beginning of the string this method
    /// is called on (argument "str" ).
    ///
    std::string lstrip( const std::string & str, const std::string & chars = "" );

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Return a copy of the string, concatenated N times, together.
    /// Corresponds to the __mul__ operator.
    /// 
    std::string mul( const std::string & str, int n);
    
    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Split the string around first occurance of sep.
    /// Three strings will always placed into result. If sep is found, the strings will
    /// be the text before sep, sep itself, and the remaining text. If sep is
    /// not found, the original string will be returned with two empty strings.
    ///
    void partition( const std::string & str, const std::string & sep, std::vector< std::string > & result );
    inline std::vector< std::string > partition( const std::string & str, const std::string & sep )
    {
        std::vector< std::string > result;
        partition( str, sep, result );
        return result;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief If str starts with prefix return a copy of the string with prefix at the start
    /// removed otherwise return an unmodified copy of the string.
    ///
    std::string removeprefix( const std::string & str, const std::string & prefix );

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief If str ends with suffix return a copy of the string with suffix at the end removed
    /// otherwise return an unmodified copy of the string.
    ///
    std::string removesuffix( const std::string & str, const std::string & suffix );

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Return a copy of the string with all occurrences of substring old replaced by new. If
    /// the optional argument count is given, only the first count occurrences are replaced.
    ///
    std::string replace( const std::string & str, const std::string & oldstr, const std::string & newstr, int count = -1);

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Return the highest index in the string where substring sub is found, such that sub is
    /// contained within s[start,end]. Optional arguments start and end are interpreted as in
    /// slice notation. Return -1 on failure.
    ///
    int rfind( const std::string & str, const std::string & sub, int start = 0, int end = MAX_32BIT_INT );

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Currently a synonym of rfind. The python version raises exceptions. This one currently
    /// does not
    ///
    int rindex( const std::string & str, const std::string & sub, int start = 0, int end = MAX_32BIT_INT );

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Return the string right justified in a string of length width. Padding is done using
    /// spaces. The original string is returned if width is less than str.size().
    ///
    std::string rjust( const std::string & str, int width);

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Split the string around last occurance of sep.
    /// Three strings will always placed into result. If sep is found, the strings will
    /// be the text before sep, sep itself, and the remaining text. If sep is
    /// not found, the original string will be returned with two empty strings.
    ///
    void rpartition( const std::string & str, const std::string & sep, std::vector< std::string > & result );
    inline std::vector< std::string > rpartition ( const std::string & str, const std::string & sep )
    {
        std::vector< std::string > result;
        rpartition( str, sep, result );
        return result;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Return a copy of the string with trailing characters removed. If chars is "", whitespace
    /// characters are removed. If not "", the characters in the string will be stripped from the
    /// end of the string this method is called on.
    ///
    std::string rstrip( const std::string & str, const std::string & chars = "" );

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Fills the "result" list with the words in the string, using sep as the delimiter string.
    /// If maxsplit is > -1, at most maxsplit splits are done. If sep is "",
    /// any whitespace string is a separator.
    ///
    void split( const std::string & str, std::vector< std::string > & result, const std::string & sep = "", int maxsplit = -1);
    inline std::vector< std::string > split( const std::string & str, const std::string & sep = "", int maxsplit = -1)
    {
        std::vector< std::string >  result;
        split( str, result, sep, maxsplit );
        return result;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Fills the "result" list with the words in the string, using sep as the delimiter string.
    /// Does a number of splits starting at the end of the string, the result still has the
    /// split strings in their original order.
    /// If maxsplit is > -1, at most maxsplit splits are done. If sep is "",
    /// any whitespace string is a separator.
    ///
    void rsplit( const std::string & str, std::vector< std::string > & result, const std::string & sep = "", int maxsplit = -1);
    inline std::vector< std::string > rsplit( const std::string & str, const std::string & sep = "", int maxsplit = -1)
    {
        std::vector< std::string > result;
        rsplit( str, result, sep, maxsplit);
        return result;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Return a list of the lines in the string, breaking at line boundaries. Line breaks
    /// are not included in the resulting list unless keepends is given and true.
    ///
    void splitlines(  const std::string & str, std::vector< std::string > & result, bool keepends = false );
    inline std::vector< std::string > splitlines(  const std::string & str, bool keepends = false )
    {
        std::vector< std::string > result;
        splitlines( str, result, keepends);
        return result;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Return True if string starts with the prefix, otherwise return False. With optional start,
    /// test string beginning at that position. With optional end, stop comparing string at that
    /// position
    ///
    bool startswith( const std::string & str, const std::string & prefix, int start = 0, int end = MAX_32BIT_INT );

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Return a copy of the string with leading and trailing characters removed. If chars is "",
    /// whitespace characters are removed. If given not "",  the characters in the string will be
    /// stripped from the both ends of the string this method is called on.
    ///
    std::string strip( const std::string & str, const std::string & chars = "" );

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Return a copy of the string with uppercase characters converted to lowercase and vice versa.
    ///
    std::string swapcase( const std::string & str );

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Return a titlecased version of the string: words start with uppercase characters,
    /// all remaining cased characters are lowercase.
    ///
    std::string title( const std::string & str );

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Return a copy of the string where all characters occurring in the optional argument
    /// deletechars are removed, and the remaining characters have been mapped through the given
    /// translation table, which must be a string of length 256.
    ///
    std::string translate( const std::string & str, const std::string & table, const std::string & deletechars = "");

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Return a copy of the string converted to uppercase.
    ///
    std::string upper( const std::string & str );

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Return the numeric string left filled with zeros in a string of length width. The original
    /// string is returned if width is less than str.size().
    ///
    std::string zfill( const std::string & str, int width );

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief function matching python's slice functionality.
    ///
    std::string slice( const std::string & str, int start = 0, int end = MAX_32BIT_INT);

    ///
    /// @ }
    ///


namespace os
{
namespace path
{
    // All of the function below have three versions.
    // Example:
    //   join(...)
    //   join_nt(...)
    //   join_posix(...)
    //
    // The regular function dispatches to the other versions - based on the OS
    // at compile time - to match the result you'd get from the python
    // interepreter on the same operating system
    // 
    // Should you want to 'lock off' to a particular version of the string
    // manipulation across *all* operating systems, use the version with the
    // _OS you are interested in.  I.e., you can use posix style path joining,
    // even on Windows, with join_posix.
    //
    // The naming, (nt, posix) matches the cpython source implementation.
    
    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @defgroup functions pystring::os::path
    /// @{
    
    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Return the base name of pathname path. This is the second half of the pair returned
    /// by split(path). Note that the result of this function is different from the Unix basename
    /// program; where basename for '/foo/bar/' returns 'bar', the basename() function returns an
    /// empty string ('').
    
    std::string basename(const std::string & path);
    std::string basename_nt(const std::string & path);
    std::string basename_posix(const std::string & path);

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Return the directory name of pathname path. This is the first half of the pair
    /// returned by split(path).
    
    std::string dirname(const std::string & path);
    std::string dirname_nt(const std::string & path);
    std::string dirname_posix(const std::string & path);

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Return True if path is an absolute pathname. On Unix, that means it begins with a
    /// slash, on Windows that it begins with a (back)slash after chopping off a potential drive
    /// letter.

    bool isabs(const std::string & path);
    bool isabs_nt(const std::string & path);
    bool isabs_posix(const std::string & s);

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Return a normalized absolutized version of the pathname path.
    /// 
    /// NOTE: This differs from the interface of the python equivalent in that it requires you
    /// to pass in the current working directory as an argument.
    
    std::string abspath(const std::string & path, const std::string & cwd);
    std::string abspath_nt(const std::string & path, const std::string & cwd);
    std::string abspath_posix(const std::string & path, const std::string & cwd);
    

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Join one or more path components intelligently. If any component is an absolute
    /// path, all previous components (on Windows, including the previous drive letter, if there
    /// was one) are thrown away, and joining continues. The return value is the concatenation of
    /// path1, and optionally path2, etc., with exactly one directory separator (os.sep) inserted
    /// between components, unless path2 is empty. Note that on Windows, since there is a current
    /// directory for each drive, os.path.join("c:", "foo") represents a path relative to the
    /// current directory on drive C: (c:foo), not c:\foo.
    
    /// This dispatches based on the compilation OS
    std::string join(const std::string & path1, const std::string & path2);
    std::string join_nt(const std::string & path1, const std::string & path2);
    std::string join_posix(const std::string & path1, const std::string & path2);
    
    std::string join(const std::vector< std::string > & paths);
    std::string join_nt(const std::vector< std::string > & paths);
    std::string join_posix(const std::vector< std::string > & paths);

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Normalize a pathname. This collapses redundant separators and up-level references
    /// so that A//B, A/B/, A/./B and A/foo/../B all become A/B. It does not normalize the case
    /// (use normcase() for that). On Windows, it converts forward slashes to backward slashes.
    /// It should be understood that this may change the meaning of the path if it contains
    /// symbolic links!

    std::string normpath(const std::string & path);
    std::string normpath_nt(const std::string & path);
    std::string normpath_posix(const std::string & path);

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Split the pathname path into a pair, (head, tail) where tail is the last pathname
    /// component and head is everything leading up to that. The tail part will never contain a
    /// slash; if path ends in a slash, tail will be empty. If there is no slash in path, head
    /// will be empty. If path is empty, both head and tail are empty. Trailing slashes are
    /// stripped from head unless it is the root (one or more slashes only). In all cases,
    /// join(head, tail) returns a path to the same location as path (but the strings may
    /// differ).

    void split(std::string & head, std::string & tail, const std::string & path);
    void split_nt(std::string & head, std::string & tail, const std::string & path);
    void split_posix(std::string & head, std::string & tail, const std::string & path);

    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Split the pathname path into a pair (drive, tail) where drive is either a drive
    /// specification or the empty string. On systems which do not use drive specifications,
    /// drive will always be the empty string. In all cases, drive + tail will be the same as
    /// path.
    
    void splitdrive(std::string & drivespec, std::string & pathspec, const std::string & path);
    void splitdrive_nt(std::string & drivespec, std::string & pathspec, const std::string & p);
    void splitdrive_posix(std::string & drivespec, std::string & pathspec, const std::string & path);
    
    //////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Split the pathname path into a pair (root, ext) such that root + ext == path, and
    /// ext is empty or begins with a period and contains at most one period. Leading periods on
    /// the basename are ignored; splitext('.cshrc') returns ('.cshrc', '').

    void splitext(std::string & root, std::string & ext, const std::string & path);
    void splitext_nt(std::string & root, std::string & ext, const std::string & path);
    void splitext_posix(std::string & root, std::string & ext, const std::string & path);
    
    ///
    /// @ }
    ///
} // namespace path
} // namespace os

} // namespace pystring




// ####################################################################
// ####################################################################
//                              Definitions
// ####################################################################
// ####################################################################
namespace pystring
{

#if defined(_WIN32) || defined(_WIN64) || defined(_WINDOWS) || defined(_MSC_VER) || defined(WINDOWS)
#define PYSTR_WINDOWS
#endif

    // This definition codes from configure.in in the python src.
    // Strictly speaking this limits us to str sizes of 2**31.
    // Should we wish to handle this limit, we could use an architecture
    // specific #defines and read from ssize_t (unistd.h) if the header exists.
    // But in the meantime, the use of int assures maximum arch compatibility.
    // This must also equal the size used in the end = MAX_32BIT_INT default arg.

    typedef int Py_ssize_t;
    const std::string forward_slash = "/";
    const std::string double_forward_slash = "//";
    const std::string triple_forward_slash = "///";
    const std::string double_back_slash = "\\";
    const std::string empty_string = "";
    const std::string dot = ".";
    const std::string double_dot = "..";
    const std::string colon = ":";


    /* helper macro to fixup start/end slice values */
#define ADJUST_INDICES(start, end, len)         \
    if (end > len)                          \
        end = len;                          \
    else if (end < 0) {                     \
        end += len;                         \
        if (end < 0)                        \
        end = 0;                        \
    }                                       \
    if (start < 0) {                        \
        start += len;                       \
        if (start < 0)                      \
        start = 0;                      \
    }


    namespace {

        //////////////////////////////////////////////////////////////////////////////////////////////
        /// why doesn't the std::reverse work?
        ///
        inline void reverse_strings(std::vector< std::string >& result)
        {
            for (std::vector< std::string >::size_type i = 0; i < result.size() / 2; i++)
            {
                std::swap(result[i], result[result.size() - 1 - i]);
            }
        }

        //////////////////////////////////////////////////////////////////////////////////////////////
        ///
        ///
        inline void split_whitespace(const std::string& str, std::vector< std::string >& result, int maxsplit)
        {
            std::string::size_type i, j, len = str.size();
            for (i = j = 0; i < len; )
            {

                while (i < len && ::isspace(str[i])) i++;
                j = i;

                while (i < len && !::isspace(str[i])) i++;



                if (j < i)
                {
                    if (maxsplit-- <= 0) break;

                    result.push_back(str.substr(j, i - j));

                    while (i < len && ::isspace(str[i])) i++;
                    j = i;
                }
            }
            if (j < len)
            {
                result.push_back(str.substr(j, len - j));
            }
        }


        //////////////////////////////////////////////////////////////////////////////////////////////
        ///
        ///
        inline void rsplit_whitespace(const std::string& str, std::vector< std::string >& result, int maxsplit)
        {
            std::string::size_type len = str.size();
            std::string::size_type i, j;
            for (i = j = len; i > 0; )
            {

                while (i > 0 && ::isspace(str[i - 1])) i--;
                j = i;

                while (i > 0 && !::isspace(str[i - 1])) i--;



                if (j > i)
                {
                    if (maxsplit-- <= 0) break;

                    result.push_back(str.substr(i, j - i));

                    while (i > 0 && ::isspace(str[i - 1])) i--;
                    j = i;
                }
            }
            if (j > 0)
            {
                result.push_back(str.substr(0, j));
            }
            //std::reverse( result, result.begin(), result.end() );
            reverse_strings(result);
        }

    } //anonymous namespace


    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline void split(const std::string& str, std::vector< std::string >& result, const std::string& sep, int maxsplit)
    {
        result.clear();

        if (maxsplit < 0) maxsplit = MAX_32BIT_INT;//result.max_size();


        if (sep.size() == 0)
        {
            split_whitespace(str, result, maxsplit);
            return;
        }

        std::string::size_type i, j, len = str.size(), n = sep.size();

        i = j = 0;

        while (i + n <= len)
        {
            if (str[i] == sep[0] && str.substr(i, n) == sep)
            {
                if (maxsplit-- <= 0) break;

                result.push_back(str.substr(j, i - j));
                i = j = i + n;
            }
            else
            {
                i++;
            }
        }

        result.push_back(str.substr(j, len - j));
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline void rsplit(const std::string& str, std::vector< std::string >& result, const std::string& sep, int maxsplit)
    {
        if (maxsplit < 0)
        {
            split(str, result, sep, maxsplit);
            return;
        }

        result.clear();

        if (sep.size() == 0)
        {
            rsplit_whitespace(str, result, maxsplit);
            return;
        }

        Py_ssize_t i, j, len = (Py_ssize_t)str.size(), n = (Py_ssize_t)sep.size();

        i = j = len;

        while (i >= n)
        {
            if (str[i - 1] == sep[n - 1] && str.substr(i - n, n) == sep)
            {
                if (maxsplit-- <= 0) break;

                result.push_back(str.substr(i, j - i));
                i = j = i - n;
            }
            else
            {
                i--;
            }
        }

        result.push_back(str.substr(0, j));
        reverse_strings(result);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
#define LEFTSTRIP 0
#define RIGHTSTRIP 1
#define BOTHSTRIP 2

//////////////////////////////////////////////////////////////////////////////////////////////
///
///
    inline std::string do_strip(const std::string& str, int striptype, const std::string& chars)
    {
        Py_ssize_t len = (Py_ssize_t)str.size(), i, j, charslen = (Py_ssize_t)chars.size();

        if (charslen == 0)
        {
            i = 0;
            if (striptype != RIGHTSTRIP)
            {
                while (i < len && ::isspace(str[i]))
                {
                    i++;
                }
            }

            j = len;
            if (striptype != LEFTSTRIP)
            {
                do
                {
                    j--;
                } while (j >= i && ::isspace(str[j]));

                j++;
            }


        }
        else
        {
            const char* sep = chars.c_str();

            i = 0;
            if (striptype != RIGHTSTRIP)
            {
                while (i < len && memchr(sep, str[i], charslen))
                {
                    i++;
                }
            }

            j = len;
            if (striptype != LEFTSTRIP)
            {
                do
                {
                    j--;
                } while (j >= i && memchr(sep, str[j], charslen));
                j++;
            }


        }

        if (i == 0 && j == len)
        {
            return str;
        }
        else
        {
            return str.substr(i, j - i);
        }

    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline void partition(const std::string& str, const std::string& sep, std::vector< std::string >& result)
    {
        result.resize(3);
        int index = find(str, sep);
        if (index < 0)
        {
            result[0] = str;
            result[1] = empty_string;
            result[2] = empty_string;
        }
        else
        {
            result[0] = str.substr(0, index);
            result[1] = sep;
            result[2] = str.substr(index + sep.size(), str.size());
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline void rpartition(const std::string& str, const std::string& sep, std::vector< std::string >& result)
    {
        result.resize(3);
        int index = rfind(str, sep);
        if (index < 0)
        {
            result[0] = empty_string;
            result[1] = empty_string;
            result[2] = str;
        }
        else
        {
            result[0] = str.substr(0, index);
            result[1] = sep;
            result[2] = str.substr(index + sep.size(), str.size());
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline std::string strip(const std::string& str, const std::string& chars)
    {
        return do_strip(str, BOTHSTRIP, chars);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline std::string lstrip(const std::string& str, const std::string& chars)
    {
        return do_strip(str, LEFTSTRIP, chars);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline std::string rstrip(const std::string& str, const std::string& chars)
    {
        return do_strip(str, RIGHTSTRIP, chars);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline std::string join(const std::string& str, const std::vector< std::string >& seq)
    {
        std::vector< std::string >::size_type seqlen = seq.size(), i;

        if (seqlen == 0) return empty_string;
        if (seqlen == 1) return seq[0];

        std::string result(seq[0]);

        for (i = 1; i < seqlen; ++i)
        {
            result += str + seq[i];

        }


        return result;
    }


    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///

    namespace
    {
        /* Matches the end (direction >= 0) or start (direction < 0) of self
         * against substr, using the start and end arguments. Returns
         * -1 on error, 0 if not found and 1 if found.
         */

        inline int _string_tailmatch(const std::string& self, const std::string& substr,
            Py_ssize_t start, Py_ssize_t end,
            int direction)
        {
            Py_ssize_t len = (Py_ssize_t)self.size();
            Py_ssize_t slen = (Py_ssize_t)substr.size();

            const char* sub = substr.c_str();
            const char* str = self.c_str();

            ADJUST_INDICES(start, end, len);

            if (direction < 0) {
                // startswith
                if (start + slen > len)
                    return 0;
            }
            else {
                // endswith
                if (end - start < slen || start > len)
                    return 0;
                if (end - slen > start)
                    start = end - slen;
            }
            if (end - start >= slen)
                return (!std::memcmp(str + start, sub, slen));

            return 0;
        }
    }

    inline bool endswith(const std::string& str, const std::string& suffix, int start, int end)
    {
        int result = _string_tailmatch(str, suffix,
            (Py_ssize_t)start, (Py_ssize_t)end, +1);
        //if (result == -1) // TODO: Error condition

        return static_cast<bool>(result);
    }


    inline bool startswith(const std::string& str, const std::string& prefix, int start, int end)
    {
        int result = _string_tailmatch(str, prefix,
            (Py_ssize_t)start, (Py_ssize_t)end, -1);
        //if (result == -1) // TODO: Error condition

        return static_cast<bool>(result);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///

    inline bool isalnum(const std::string& str)
    {
        std::string::size_type len = str.size(), i;
        if (len == 0) return false;


        if (len == 1)
        {
            return ::isalnum(str[0]);
        }

        for (i = 0; i < len; ++i)
        {
            if (!::isalnum(str[i])) return false;
        }
        return true;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline bool isalpha(const std::string& str)
    {
        std::string::size_type len = str.size(), i;
        if (len == 0) return false;
        if (len == 1) return ::isalpha((int)str[0]);

        for (i = 0; i < len; ++i)
        {
            if (!::isalpha((int)str[i])) return false;
        }
        return true;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline bool isdigit(const std::string& str)
    {
        std::string::size_type len = str.size(), i;
        if (len == 0) return false;
        if (len == 1) return ::isdigit(str[0]);

        for (i = 0; i < len; ++i)
        {
            if (!::isdigit(str[i])) return false;
        }
        return true;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline bool islower(const std::string& str)
    {
        std::string::size_type len = str.size(), i;
        if (len == 0) return false;
        if (len == 1) return ::islower(str[0]);

        for (i = 0; i < len; ++i)
        {
            if (!::islower(str[i])) return false;
        }
        return true;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline bool isspace(const std::string& str)
    {
        std::string::size_type len = str.size(), i;
        if (len == 0) return false;
        if (len == 1) return ::isspace(str[0]);

        for (i = 0; i < len; ++i)
        {
            if (!::isspace(str[i])) return false;
        }
        return true;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline bool istitle(const std::string& str)
    {
        std::string::size_type len = str.size(), i;

        if (len == 0) return false;
        if (len == 1) return ::isupper(str[0]);

        bool cased = false, previous_is_cased = false;

        for (i = 0; i < len; ++i)
        {
            if (::isupper(str[i]))
            {
                if (previous_is_cased)
                {
                    return false;
                }

                previous_is_cased = true;
                cased = true;
            }
            else if (::islower(str[i]))
            {
                if (!previous_is_cased)
                {
                    return false;
                }

                previous_is_cased = true;
                cased = true;

            }
            else
            {
                previous_is_cased = false;
            }
        }

        return cased;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline bool isupper(const std::string& str)
    {
        std::string::size_type len = str.size(), i;
        if (len == 0) return false;
        if (len == 1) return ::isupper(str[0]);

        for (i = 0; i < len; ++i)
        {
            if (!::isupper(str[i])) return false;
        }
        return true;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline std::string capitalize(const std::string& str)
    {
        std::string s(str);
        std::string::size_type len = s.size(), i;

        if (len > 0)
        {
            if (::islower(s[0])) s[0] = (char) ::toupper(s[0]);
        }

        for (i = 1; i < len; ++i)
        {
            if (::isupper(s[i])) s[i] = (char) ::tolower(s[i]);
        }

        return s;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline std::string lower(const std::string& str)
    {
        std::string s(str);
        std::string::size_type len = s.size(), i;

        for (i = 0; i < len; ++i)
        {
            if (::isupper(s[i])) s[i] = (char) ::tolower(s[i]);
        }

        return s;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline std::string upper(const std::string& str)
    {
        std::string s(str);
        std::string::size_type len = s.size(), i;

        for (i = 0; i < len; ++i)
        {
            if (::islower(s[i])) s[i] = (char) ::toupper(s[i]);
        }

        return s;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline std::string swapcase(const std::string& str)
    {
        std::string s(str);
        std::string::size_type len = s.size(), i;

        for (i = 0; i < len; ++i)
        {
            if (::islower(s[i])) s[i] = (char) ::toupper(s[i]);
            else if (::isupper(s[i])) s[i] = (char) ::tolower(s[i]);
        }

        return s;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline std::string title(const std::string& str)
    {
        std::string s(str);
        std::string::size_type len = s.size(), i;
        bool previous_is_cased = false;

        for (i = 0; i < len; ++i)
        {
            int c = s[i];
            if (::islower(c))
            {
                if (!previous_is_cased)
                {
                    s[i] = (char) ::toupper(c);
                }
                previous_is_cased = true;
            }
            else if (::isupper(c))
            {
                if (previous_is_cased)
                {
                    s[i] = (char) ::tolower(c);
                }
                previous_is_cased = true;
            }
            else
            {
                previous_is_cased = false;
            }
        }

        return s;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline std::string translate(const std::string& str, const std::string& table, const std::string& deletechars)
    {
        std::string s;
        std::string::size_type len = str.size(), dellen = deletechars.size();

        if (table.size() != 256)
        {
            // TODO : raise exception instead
            return str;
        }

        //if nothing is deleted, use faster code
        if (dellen == 0)
        {
            s = str;
            for (std::string::size_type i = 0; i < len; ++i)
            {
                s[i] = table[s[i]];
            }
            return s;
        }


        int trans_table[256];
        for (int i = 0; i < 256; i++)
        {
            trans_table[i] = table[i];
        }

        for (std::string::size_type i = 0; i < dellen; i++)
        {
            trans_table[(int)deletechars[i]] = -1;
        }

        for (std::string::size_type i = 0; i < len; ++i)
        {
            if (trans_table[(int)str[i]] != -1)
            {
                s += table[str[i]];
            }
        }

        return s;

    }


    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline std::string zfill(const std::string& str, int width)
    {
        int len = (int)str.size();

        if (len >= width)
        {
            return str;
        }

        std::string s(str);

        int fill = width - len;

        s = std::string(fill, '0') + s;


        if (s[fill] == '+' || s[fill] == '-')
        {
            s[0] = s[fill];
            s[fill] = '0';
        }

        return s;

    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline std::string ljust(const std::string& str, int width)
    {
        std::string::size_type len = str.size();
        if (((int)len) >= width) return str;
        return str + std::string(width - len, ' ');
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline std::string rjust(const std::string& str, int width)
    {
        std::string::size_type len = str.size();
        if (((int)len) >= width) return str;
        return std::string(width - len, ' ') + str;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline std::string center(const std::string& str, int width)
    {
        int len = (int)str.size();
        int marg, left;

        if (len >= width) return str;

        marg = width - len;
        left = marg / 2 + (marg & width & 1);

        return std::string(left, ' ') + str + std::string(marg - left, ' ');

    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline std::string slice(const std::string& str, int start, int end)
    {
        ADJUST_INDICES(start, end, (int)str.size());
        if (start >= end) return empty_string;
        return str.substr(start, end - start);
    }


    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline int find(const std::string& str, const std::string& sub, int start, int end)
    {
        ADJUST_INDICES(start, end, (int)str.size());

        std::string::size_type result = str.find(sub, start);

        // If we cannot find the string, or if the end-point of our found substring is past
        // the allowed end limit, return that it can't be found.
        if (result == std::string::npos ||
            (result + sub.size() > (std::string::size_type)end))
        {
            return -1;
        }

        return (int)result;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline int index(const std::string& str, const std::string& sub, int start, int end)
    {
        return find(str, sub, start, end);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline int rfind(const std::string& str, const std::string& sub, int start, int end)
    {
        ADJUST_INDICES(start, end, (int)str.size());

        std::string::size_type result = str.rfind(sub, end);

        if (result == std::string::npos ||
            result < (std::string::size_type)start ||
            (result + sub.size() > (std::string::size_type)end))
            return -1;

        return (int)result;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline int rindex(const std::string& str, const std::string& sub, int start, int end)
    {
        return rfind(str, sub, start, end);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline std::string expandtabs(const std::string& str, int tabsize)
    {
        std::string s(str);

        std::string::size_type len = str.size(), i = 0;
        int offset = 0;

        int j = 0;

        for (i = 0; i < len; ++i)
        {
            if (str[i] == '\t')
            {

                if (tabsize > 0)
                {
                    int fillsize = tabsize - (j % tabsize);
                    j += fillsize;
                    s.replace(i + offset, 1, std::string(fillsize, ' '));
                    offset += fillsize - 1;
                }
                else
                {
                    s.replace(i + offset, 1, empty_string);
                    offset -= 1;
                }

            }
            else
            {
                j++;

                if (str[i] == '\n' || str[i] == '\r')
                {
                    j = 0;
                }
            }
        }

        return s;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline int count(const std::string& str, const std::string& substr, int start, int end)
    {
        int nummatches = 0;
        int cursor = start;

        while (1)
        {
            cursor = find(str, substr, cursor, end);

            if (cursor < 0) break;

            cursor += (int)substr.size();
            nummatches += 1;
        }

        return nummatches;


    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///

    inline std::string replace(const std::string& str, const std::string& oldstr, const std::string& newstr, int count)
    {
        int sofar = 0;
        int cursor = 0;
        std::string s(str);

        std::string::size_type oldlen = oldstr.size(), newlen = newstr.size();

        cursor = find(s, oldstr, cursor);

        while (cursor != -1 && cursor <= (int)s.size())
        {
            if (count > -1 && sofar >= count)
            {
                break;
            }

            s.replace(cursor, oldlen, newstr);
            cursor += (int)newlen;

            if (oldlen != 0)
            {
                cursor = find(s, oldstr, cursor);
            }
            else
            {
                ++cursor;
            }

            ++sofar;
        }

        return s;

    }


    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline void splitlines(const std::string& str, std::vector< std::string >& result, bool keepends)
    {
        result.clear();
        std::string::size_type len = str.size(), i, j, eol;

        for (i = j = 0; i < len; )
        {
            while (i < len && str[i] != '\n' && str[i] != '\r') i++;

            eol = i;
            if (i < len)
            {
                if (str[i] == '\r' && i + 1 < len && str[i + 1] == '\n')
                {
                    i += 2;
                }
                else
                {
                    i++;
                }
                if (keepends)
                    eol = i;

            }

            result.push_back(str.substr(j, eol - j));
            j = i;

        }

        if (j < len)
        {
            result.push_back(str.substr(j, len - j));
        }

    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline std::string mul(const std::string& str, int n)
    {
        // Early exits
        if (n <= 0) return empty_string;
        if (n == 1) return str;

        std::ostringstream os;
        for (int i = 0; i < n; ++i)
        {
            os << str;
        }
        return os.str();
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline std::string removeprefix(const std::string& str, const std::string& prefix)
    {
        if (pystring::startswith(str, prefix))
        {
            return str.substr(prefix.length());
        }

        return str;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    inline std::string removesuffix(const std::string& str, const std::string& suffix)
    {
        if (pystring::endswith(str, suffix))
        {
            return str.substr(0, str.length() - suffix.length());
        }

        return str;
    }


    namespace os
    {
        namespace path
        {

            //////////////////////////////////////////////////////////////////////////////////////////////
            ///
            ///
            /// These functions are C++ ports of the python2.6 versions of os.path,
            /// and come from genericpath.py, ntpath.py, posixpath.py

            /// Split a pathname into drive and path specifiers.
            /// Returns drivespec, pathspec. Either part may be empty.
            inline void splitdrive_nt(std::string& drivespec, std::string& pathspec,
                const std::string& p)
            {
                if (p.size() >= 2 && p[1] == ':')
                {
                    std::string path = p; // In case drivespec == p
                    drivespec = pystring::slice(path, 0, 2);
                    pathspec = pystring::slice(path, 2);
                }
                else
                {
                    drivespec = empty_string;
                    pathspec = p;
                }
            }

            // On Posix, drive is always empty
            inline void splitdrive_posix(std::string& drivespec, std::string& pathspec,
                const std::string& path)
            {
                drivespec = empty_string;
                pathspec = path;
            }

            inline void splitdrive(std::string& drivespec, std::string& pathspec,
                const std::string& path)
            {
#ifdef PYSTR_WINDOWS
                return splitdrive_nt(drivespec, pathspec, path);
#else
                return splitdrive_posix(drivespec, pathspec, path);
#endif
            }

            //////////////////////////////////////////////////////////////////////////////////////////////
            ///
            ///

            // Test whether a path is absolute
            // In windows, if the character to the right of the colon
            // is a forward or backslash it's absolute.
            inline bool isabs_nt(const std::string& path)
            {
                std::string drivespec, pathspec;
                splitdrive_nt(drivespec, pathspec, path);
                if (pathspec.empty()) return false;
                return ((pathspec[0] == '/') || (pathspec[0] == '\\'));
            }

            inline bool isabs_posix(const std::string& s)
            {
                return pystring::startswith(s, forward_slash);
            }

            inline bool isabs(const std::string& path)
            {
#ifdef PYSTR_WINDOWS
                return isabs_nt(path);
#else
                return isabs_posix(path);
#endif
            }


            //////////////////////////////////////////////////////////////////////////////////////////////
            ///
            ///

            inline std::string abspath_nt(const std::string& path, const std::string& cwd)
            {
                std::string p = path;
                if (!isabs_nt(p)) p = join_nt(cwd, p);
                return normpath_nt(p);
            }

            inline std::string abspath_posix(const std::string& path, const std::string& cwd)
            {
                std::string p = path;
                if (!isabs_posix(p)) p = join_posix(cwd, p);
                return normpath_posix(p);
            }

            inline std::string abspath(const std::string& path, const std::string& cwd)
            {
#ifdef PYSTR_WINDOWS
                return abspath_nt(path, cwd);
#else
                return abspath_posix(path, cwd);
#endif
            }


            //////////////////////////////////////////////////////////////////////////////////////////////
            ///
            ///

            inline std::string join_nt(const std::vector< std::string >& paths)
            {
                if (paths.empty()) return empty_string;
                if (paths.size() == 1) return paths[0];

                std::string path = paths[0];

                for (unsigned int i = 1; i < paths.size(); ++i)
                {
                    std::string b = paths[i];

                    bool b_nts = false;
                    if (path.empty())
                    {
                        b_nts = true;
                    }
                    else if (isabs_nt(b))
                    {
                        // This probably wipes out path so far.  However, it's more
                        // complicated if path begins with a drive letter:
                        //     1. join('c:', '/a') == 'c:/a'
                        //     2. join('c:/', '/a') == 'c:/a'
                        // But
                        //     3. join('c:/a', '/b') == '/b'
                        //     4. join('c:', 'd:/') = 'd:/'
                        //     5. join('c:/', 'd:/') = 'd:/'


                        if ((path.size() >= 2 && path[1] != ':') || (b.size() >= 2 && b[1] == ':'))
                        {
                            // Path doesnt start with a drive letter
                            b_nts = true;
                        }
                        // Else path has a drive letter, and b doesn't but is absolute.
                        else if ((path.size() > 3) ||
                            ((path.size() == 3) && !pystring::endswith(path, forward_slash) && !pystring::endswith(path, double_back_slash)))
                        {
                            b_nts = true;
                        }
                    }

                    if (b_nts)
                    {
                        path = b;
                    }
                    else
                    {
                        // Join, and ensure there's a separator.
                        // assert len(path) > 0
                        if (pystring::endswith(path, forward_slash) || pystring::endswith(path, double_back_slash))
                        {
                            if (pystring::startswith(b, forward_slash) || pystring::startswith(b, double_back_slash))
                            {
                                path += pystring::slice(b, 1);
                            }
                            else
                            {
                                path += b;
                            }
                        }
                        else if (pystring::endswith(path, colon))
                        {
                            path += b;
                        }
                        else if (!b.empty())
                        {
                            if (pystring::startswith(b, forward_slash) || pystring::startswith(b, double_back_slash))
                            {
                                path += b;
                            }
                            else
                            {
                                path += double_back_slash + b;
                            }
                        }
                        else
                        {
                            // path is not empty and does not end with a backslash,
                            // but b is empty; since, e.g., split('a/') produces
                            // ('a', ''), it's best if join() adds a backslash in
                            // this case.
                            path += double_back_slash;
                        }
                    }
                }

                return path;
            }

            // Join two or more pathname components, inserting double_back_slash as needed.
            inline std::string join_nt(const std::string& a, const std::string& b)
            {
                std::vector< std::string > paths(2);
                paths[0] = a;
                paths[1] = b;
                return join_nt(paths);
            }

            // Join pathnames.
            // If any component is an absolute path, all previous path components
            // will be discarded.
            // Ignore the previous parts if a part is absolute.
            // Insert a '/' unless the first part is empty or already ends in '/'.

            inline std::string join_posix(const std::vector< std::string >& paths)
            {
                if (paths.empty()) return empty_string;
                if (paths.size() == 1) return paths[0];

                std::string path = paths[0];

                for (unsigned int i = 1; i < paths.size(); ++i)
                {
                    std::string b = paths[i];
                    if (pystring::startswith(b, forward_slash))
                    {
                        path = b;
                    }
                    else if (path.empty() || pystring::endswith(path, forward_slash))
                    {
                        path += b;
                    }
                    else
                    {
                        path += forward_slash + b;
                    }
                }

                return path;
            }

            inline std::string join_posix(const std::string& a, const std::string& b)
            {
                std::vector< std::string > paths(2);
                paths[0] = a;
                paths[1] = b;
                return join_posix(paths);
            }

            inline std::string join(const std::string& path1, const std::string& path2)
            {
#ifdef PYSTR_WINDOWS
                return join_nt(path1, path2);
#else
                return join_posix(path1, path2);
#endif
            }


            inline std::string join(const std::vector< std::string >& paths)
            {
#ifdef PYSTR_WINDOWS
                return join_nt(paths);
#else
                return join_posix(paths);
#endif
            }

            //////////////////////////////////////////////////////////////////////////////////////////////
            ///
            ///


            // Split a pathname.
            // Return (head, tail) where tail is everything after the final slash.
            // Either part may be empty

            inline void split_nt(std::string& head, std::string& tail, const std::string& path)
            {
                std::string d, p;
                splitdrive_nt(d, p, path);

                // set i to index beyond p's last slash
                int i = (int)p.size();

                // walk back to find the index of the first slash from the end       
                while (i > 0 && (p[i - 1] != '\\') && (p[i - 1] != '/'))
                {
                    i = i - 1;
                }

                head = pystring::slice(p, 0, i);
                tail = pystring::slice(p, i); // now tail has no slashes

                // remove trailing slashes from head, unless it's all slashes
                std::string head2 = head;
                while (!head2.empty() && ((pystring::slice(head2, -1) == forward_slash) ||
                    (pystring::slice(head2, -1) == double_back_slash)))
                {
                    head2 = pystring::slice(head2, 0, -1);
                }

                if (!head2.empty()) head = head2;
                head = d + head;
            }


            // Split a path in head (everything up to the last '/') and tail (the
            // rest).  If the path ends in '/', tail will be empty.  If there is no
            // '/' in the path, head  will be empty.
            // Trailing '/'es are stripped from head unless it is the root.

            inline void split_posix(std::string& head, std::string& tail, const std::string& p)
            {
                int i = pystring::rfind(p, forward_slash) + 1;

                head = pystring::slice(p, 0, i);
                tail = pystring::slice(p, i);

                if (!head.empty() && (head != pystring::mul(forward_slash, (int)head.size())))
                {
                    head = pystring::rstrip(head, forward_slash);
                }
            }

            inline void split(std::string& head, std::string& tail, const std::string& path)
            {
#ifdef PYSTR_WINDOWS
                return split_nt(head, tail, path);
#else
                return split_posix(head, tail, path);
#endif
            }


            //////////////////////////////////////////////////////////////////////////////////////////////
            ///
            ///

            inline std::string basename_nt(const std::string& path)
            {
                std::string head, tail;
                split_nt(head, tail, path);
                return tail;
            }

            inline std::string basename_posix(const std::string& path)
            {
                std::string head, tail;
                split_posix(head, tail, path);
                return tail;
            }

            inline std::string basename(const std::string& path)
            {
#ifdef PYSTR_WINDOWS
                return basename_nt(path);
#else
                return basename_posix(path);
#endif
            }

            inline std::string dirname_nt(const std::string& path)
            {
                std::string head, tail;
                split_nt(head, tail, path);
                return head;
            }

            inline std::string dirname_posix(const std::string& path)
            {
                std::string head, tail;
                split_posix(head, tail, path);
                return head;
            }

            inline std::string dirname(const std::string& path)
            {
#ifdef PYSTR_WINDOWS
                return dirname_nt(path);
#else
                return dirname_posix(path);
#endif
            }


            //////////////////////////////////////////////////////////////////////////////////////////////
            ///
            ///

            // Normalize a path, e.g. A//B, A/./B and A/foo/../B all become A\B.
            inline std::string normpath_nt(const std::string& p)
            {
                std::string path = p;
                path = pystring::replace(path, forward_slash, double_back_slash);

                std::string prefix;
                splitdrive_nt(prefix, path, path);

                // We need to be careful here. If the prefix is empty, and the path starts
                // with a backslash, it could either be an absolute path on the current
                // drive (\dir1\dir2\file) or a UNC filename (\\server\mount\dir1\file). It
                // is therefore imperative NOT to collapse multiple backslashes blindly in
                // that case.
                // The code below preserves multiple backslashes when there is no drive
                // letter. This means that the invalid filename \\\a\b is preserved
                // unchanged, where a\\\b is normalised to a\b. It's not clear that there
                // is any better behaviour for such edge cases.

                if (prefix.empty())
                {
                    // No drive letter - preserve initial backslashes
                    while (pystring::slice(path, 0, 1) == double_back_slash)
                    {
                        prefix = prefix + double_back_slash;
                        path = pystring::slice(path, 1);
                    }
                }
                else
                {
                    // We have a drive letter - collapse initial backslashes
                    if (pystring::startswith(path, double_back_slash))
                    {
                        prefix = prefix + double_back_slash;
                        path = pystring::lstrip(path, double_back_slash);
                    }
                }

                std::vector<std::string> comps;
                pystring::split(path, comps, double_back_slash);

                int i = 0;

                while (i < (int)comps.size())
                {
                    if (comps[i].empty() || comps[i] == dot)
                    {
                        comps.erase(comps.begin() + i);
                    }
                    else if (comps[i] == double_dot)
                    {
                        if (i > 0 && comps[i - 1] != double_dot)
                        {
                            comps.erase(comps.begin() + i - 1, comps.begin() + i + 1);
                            i -= 1;
                        }
                        else if (i == 0 && pystring::endswith(prefix, double_back_slash))
                        {
                            comps.erase(comps.begin() + i);
                        }
                        else
                        {
                            i += 1;
                        }
                    }
                    else
                    {
                        i += 1;
                    }
                }

                // If the path is now empty, substitute '.'
                if (prefix.empty() && comps.empty())
                {
                    comps.push_back(dot);
                }

                return prefix + pystring::join(double_back_slash, comps);
            }

            // Normalize a path, e.g. A//B, A/./B and A/foo/../B all become A/B.
            // It should be understood that this may change the meaning of the path
            // if it contains symbolic links!
            // Normalize path, eliminating double slashes, etc.

            inline std::string normpath_posix(const std::string& p)
            {
                if (p.empty()) return dot;

                std::string path = p;

                int initial_slashes = pystring::startswith(path, forward_slash) ? 1 : 0;

                // POSIX allows one or two initial slashes, but treats three or more
                // as single slash.

                if (initial_slashes && pystring::startswith(path, double_forward_slash)
                    && !pystring::startswith(path, triple_forward_slash))
                    initial_slashes = 2;

                std::vector<std::string> comps, new_comps;
                pystring::split(path, comps, forward_slash);

                for (unsigned int i = 0; i < comps.size(); ++i)
                {
                    std::string comp = comps[i];
                    if (comp.empty() || comp == dot)
                        continue;

                    if ((comp != double_dot) || ((initial_slashes == 0) && new_comps.empty()) ||
                        (!new_comps.empty() && new_comps[new_comps.size() - 1] == double_dot))
                    {
                        new_comps.push_back(comp);
                    }
                    else if (!new_comps.empty())
                    {
                        new_comps.pop_back();
                    }
                }

                path = pystring::join(forward_slash, new_comps);

                if (initial_slashes > 0)
                    path = pystring::mul(forward_slash, initial_slashes) + path;

                if (path.empty()) return dot;
                return path;
            }

            inline std::string normpath(const std::string& path)
            {
#ifdef PYSTR_WINDOWS
                return normpath_nt(path);
#else
                return normpath_posix(path);
#endif
            }

            //////////////////////////////////////////////////////////////////////////////////////////////
            ///
            ///

            // Split the extension from a pathname.
            // Extension is everything from the last dot to the end, ignoring
            // leading dots.  Returns "(root, ext)"; ext may be empty.
            // It is always true that root + ext == p

            inline void splitext_generic(std::string& root, std::string& ext,
                const std::string& p,
                const std::string& sep,
                const std::string& altsep,
                const std::string& extsep)
            {
                int sepIndex = pystring::rfind(p, sep);
                if (!altsep.empty())
                {
                    int altsepIndex = pystring::rfind(p, altsep);
                    sepIndex = std::max(sepIndex, altsepIndex);
                }

                int dotIndex = pystring::rfind(p, extsep);
                if (dotIndex > sepIndex)
                {
                    // Skip all leading dots
                    int filenameIndex = sepIndex + 1;

                    while (filenameIndex < dotIndex)
                    {
                        if (pystring::slice(p, filenameIndex) != extsep)
                        {
                            root = pystring::slice(p, 0, dotIndex);
                            ext = pystring::slice(p, dotIndex);
                            return;
                        }

                        filenameIndex += 1;
                    }
                }

                root = p;
                ext = empty_string;
            }

            inline void splitext_nt(std::string& root, std::string& ext, const std::string& path)
            {
                return splitext_generic(root, ext, path,
                    double_back_slash, forward_slash, dot);
            }

            inline void splitext_posix(std::string& root, std::string& ext, const std::string& path)
            {
                return splitext_generic(root, ext, path,
                    forward_slash, empty_string, dot);
            }

            inline void splitext(std::string& root, std::string& ext, const std::string& path)
            {
#ifdef PYSTR_WINDOWS
                return splitext_nt(root, ext, path);
#else
                return splitext_posix(root, ext, path);
#endif
            }

        } // namespace path
    } // namespace os


}//namespace pystring



#endif
