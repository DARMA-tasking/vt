
#if !defined INCLUDED_VT_CONFIGS_DEBUG_DEBUG_COLORIZE_H
#define INCLUDED_VT_CONFIGS_DEBUG_DEBUG_COLORIZE_H

#include "vt/configs/arguments/args.h"
#include "vt/configs/types/types_type.h"

#include <string>
#include <unistd.h>
// #include <sys/stat.h>

namespace vt { namespace debug {

inline auto istty() -> bool {
  return isatty(fileno(stdout)) ? true : false;
}

inline auto ttyc() -> bool {
  auto nocolor = arguments::ArgConfig::vt_no_color ? false : true;
  auto tty = arguments::ArgConfig::vt_auto_color ? istty() : nocolor;
  return tty;
}

inline auto green()    -> std::string { return ttyc() ? "\33[32m"   : ""; };
inline auto bold()     -> std::string { return ttyc() ? "\033[1m"   : ""; };
inline auto magenta()  -> std::string { return ttyc() ? "\33[95m"   : ""; };
inline auto red()      -> std::string { return ttyc() ? "\033[31m"  : ""; };
inline auto bred()     -> std::string { return ttyc() ? "\33[31;1m" : ""; };
inline auto reset()    -> std::string { return ttyc() ? "\33[00m"   : ""; };
inline auto bd_green() -> std::string { return ttyc() ? "\33[32;1m" : ""; };
inline auto it_green() -> std::string { return ttyc() ? "\33[32;3m" : ""; };
inline auto un_green() -> std::string { return ttyc() ? "\33[32;4m" : ""; };
inline auto byellow()  -> std::string { return ttyc() ? "\33[33;1m" : ""; };
inline auto yellow()   -> std::string { return ttyc() ? "\33[33m"   : ""; };
inline auto blue()     -> std::string { return ttyc() ? "\33[34m"   : ""; };

inline auto emph(std::string str) -> std::string  {
  return magenta() + str + reset();
};
inline auto reg(std::string str) -> std::string  {
  return green() + str + reset();
};
inline auto vtPre() -> std::string  {
  return bd_green() + std::string("vt") + reset() + ": ";
};
inline auto proc(vt::NodeType const& node) -> std::string  {
  return blue() + "[" + std::to_string(node) + "]" + reset();
};

// static bool ttyi(FILE* stream) {
//   struct stat stream_stat;
//   if (fstat(fileno(stream), &stream_stat) == 0) {
//     if (stream_stat.st_mode & S_IFREG) {
//       return true;
//     }
//   }
//   return false;
// }

}} /* end namespace vt::debug */


#endif /*INCLUDED_VT_CONFIGS_DEBUG_DEBUG_COLORIZE_H*/
