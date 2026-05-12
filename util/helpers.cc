#include "helpers.hh"

#include <cctype>
#include <sstream>

using namespace std;

string pretty_print( string_view str, size_t max_length )
{
  ostringstream ss;
  size_t shown = 0;
  for ( unsigned char ch : str ) {
    if ( shown >= max_length ) {
      ss << "...";
      break;
    }
    if ( isprint( ch ) ) {
      ss << static_cast<char>( ch );
    } else {
      ss << "\\x";
      const char* hex = "0123456789ABCDEF";
      ss << hex[( ch >> 4 ) & 0xF] << hex[ch & 0xF];
    }
    ++shown;
  }
  return ss.str();
}
