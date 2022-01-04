// Copyright (C) 2005-2009 by Jukka Korpela
// Copyright (C) 2009-2013 by David Hoerl
// Copyright (C) 2013 by Martin Moene
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef ENG_FORMAT_H_INCLUDED
#define ENG_FORMAT_H_INCLUDED

#include <string>

/**
 * convert a double to the specified number of digits in SI (prefix) or
 * exponential notation, optionally followed by a unit.
 */
std::string
to_engineering_string( double value, int digits, bool exponential, std::string unit = "", std::string separator = " " );

/**
 * convert the output of to_engineering_string() into a double.
 */
double
from_engineering_string( std::string text );

/**
 * step a value by the smallest possible increment.
 */
std::string
step_engineering_string( std::string text, int digits, bool exponential, bool increment );

//
// Extended interface:
//

/**
 * \var eng_prefixed
 * \brief select SI (prefix) presentation: to_engineering_string(), step_engineering_string().
 */

/**
 * \var eng_exponential
 * \brief select exponential presentation: to_engineering_string(), step_engineering_string().
 */

extern struct eng_prefixed_t {} eng_prefixed;
extern struct eng_exponential_t {} eng_exponential;

/**
 * \var eng_increment
 * \brief let step_engineering_string() make a postive step.
 */

/**
 * \var eng_decrement
 * \brief let step_engineering_string() make a negative step.
 */

constexpr bool eng_increment = true;
constexpr bool eng_decrement = false;

/**
 * convert a double to the specified number of digits in SI (prefix) notation,
 * optionally followed by a unit.
 */
inline std::string
to_engineering_string( double value, int digits, eng_prefixed_t, std::string unit = "", std::string separator = " " )
{
    return to_engineering_string( value, digits, false, unit, separator );
}

/**
 * convert a double to the specified number of digits in exponential notation,
 * optionally followed by a unit.
 */
inline std::string
to_engineering_string( double value, int digits, eng_exponential_t, std::string unit = "", std::string separator = " " )
{
    return to_engineering_string( value, digits, true, unit, separator );
}

/**
 * step a value by the smallest possible increment, using SI notation.
 */
inline std::string
step_engineering_string( std::string text, int digits, eng_prefixed_t, bool increment )
{
    return step_engineering_string( text, digits, false, increment );
}

/**
 * step a value by the smallest possible increment, using exponential notation.
 */
inline std::string
step_engineering_string( std::string text, int digits, eng_exponential_t, bool increment )
{
    return step_engineering_string( text, digits, true, increment );
}

#endif // ENG_FORMAT_H_INCLUDED
