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

#include "eng_format.hpp"

#include <cctype>
#include <cfloat>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>

/*
 * Note: micro, µ, may not work everywhere, so you can define a glyph yourself:
 */
#ifndef ENG_FORMAT_MICRO_GLYPH
# define ENG_FORMAT_MICRO_GLYPH "µ"
#endif

/*
 * Note: if not using signed at the computation of prefix_end below,
 * VC2010 -Wall issues a warning about unsigned and addition overflow.
 * Hence the cast to signed int here.
 */
#define ENG_FORMAT_DIMENSION_OF(a) ( static_cast<int>( sizeof(a) / sizeof(0[a]) ) )

eng_prefixed_t eng_prefixed;
eng_exponential_t eng_exponential;

namespace
{

char const * const prefixes[/*exp*/][2][9] =
{
    {
        {   "",   "m",   ENG_FORMAT_MICRO_GLYPH
                            ,   "n",    "p",    "f",    "a",    "z",    "y", },
        {   "",   "k",   "M",   "G",    "T",    "P",    "E",    "Z",    "Y", },
    },
    {
        { "e0", "e-3", "e-6", "e-9", "e-12", "e-15", "e-18", "e-21", "e-24", },
        { "e0",  "e3",  "e6",  "e9",  "e12",  "e15",  "e18",  "e21",  "e24", },
    },
};

const int prefix_count = ENG_FORMAT_DIMENSION_OF( prefixes[false][false]  );

int sign( int const value )
{
    return value == 0 ? +1 : value / std::abs( value );
}

bool is_zero( double const value )
{
    return FP_ZERO == std::fpclassify( value );
}

long degree_of( double const value )
{
    return is_zero( value ) ?
        0 : std::lrint( std::floor( std::log10( std::fabs( value ) ) / 3) );
}

int precision( double const scaled, int const digits )
{
    // MSVC6 requires -2 * DBL_EPSILON;
    // g++ 4.8.1: ok with -1 * DBL_EPSILON

    return is_zero( scaled ) ?
        digits - 1 : digits - std::log10( std::fabs( scaled ) ) - 2 * DBL_EPSILON;
}

std::string prefix_or_exponent( bool const exponential, int const degree, std::string separator )
{
    return std::string( exponential || 0 == degree ? "" : separator ) + prefixes[ exponential ][ sign(degree) > 0 ][ std::abs( degree ) ];
}

std::string exponent( int const degree )
{
    std::ostringstream os;
    os << "e" << 3 * degree;
    return os.str();
}

char const * first_non_space( char const * text )
{
    while ( *text && std::isspace( *text ) )
    {
        ++text;
    }
    return text;
}

bool starts_with( std::string const text, std::string const start )
{
    return 0 == text.find( start );
}

/*
 * "k" => 3
 */
int prefix_to_exponent( std::string const pfx )
{
    for ( int i = 0; i < 2; ++i )
    {
        // skip prefixes[0][i][0], it matches everything
        for( int k = 1; k < prefix_count; ++k )
        {
            if ( starts_with( pfx, prefixes[0][i][k] ) )
            {
                return ( i ? 1 : -1 ) * k * 3;
            }
        }
    }
    return 0;
}

} // anonymous namespace

/**
 * convert real number to prefixed or exponential notation, optionally followed by a unit.
 */
std::string
to_engineering_string( double const value, int const digits, bool exponential, std::string const unit /*= ""*/, std::string separator /*= " "*/ )
{
    if      ( std::isnan( value ) ) return "NaN";
    else if ( std::isinf( value ) ) return "INFINITE";

    const int degree = degree_of( value );

    std::string factor;

    if ( std::abs( degree ) < prefix_count )
    {
        factor = prefix_or_exponent( exponential, degree, separator );
    }
    else
    {
        exponential = true;
        factor = exponent( degree );
    }

    std::ostringstream os;

    const double scaled = value * std::pow( 1000.0, -degree );

    const std::string space = ( 0 == degree || exponential ) && unit.length() ? separator : "";

    os << std::fixed << std::setprecision( precision( scaled, digits ) ) << scaled << factor << space << unit;

    return os.str();
}

/**
 * convert the output of to_engineering_string() into a double.
 *
 * The engineering presentation should not contain a unit, as the first letter
 * is interpreted as an SI prefix, e.g. "1 T" is 1e12, not 1 (Tesla).
 *
 * "1.23 M"   => 1.23e+6
 * "1.23 kPa" => 1.23e+3  (ok, but not recommended)
 * "1.23 Pa"  => 1.23e+12 (not what's intended!)
 */
double from_engineering_string( std::string const text )
{
    char * tail;
    const double magnitude = strtod( text.c_str(), &tail );

    return magnitude * std::pow( 10.0, prefix_to_exponent( first_non_space( tail ) ) );
}

/**
 * step a value by the smallest possible increment.
 */
std::string step_engineering_string( std::string const text, int digits, bool const exponential, bool const positive )
{
    const double value = from_engineering_string( text );

    if ( digits < 3 )
    {
        digits = 3;
    }

    // correctly round to desired precision
    const int expof10 = is_zero(value) ?
        0 : std::lrint( std::floor( std::log10( value ) ) );
    const int   power = expof10 + 1 - digits;

    const double  inc = std::pow( 10.0, power ) * ( positive ? +1 : -1 );
    const double  ret = value + inc;

    return to_engineering_string( ret, digits, exponential );
}

// end of file
