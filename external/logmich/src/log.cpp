/*
**  LogMich - A Trivial Logging Library
**  Copyright (C) 2014 Ingo Ruhnke <grumbel@gmail.com>
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "log.hpp"

#include <iostream>

Logger g_logger;

std::string log_pretty_print(const std::string& str)
{
  // FIXME: very basic, might not work with complex return types
  std::string::size_type function_start = 0;
  for(std::string::size_type i = 0; i < str.size(); ++i)
  {
    if (str[i] == ' ')
    {
      function_start = i+1;
    }
    else if (str[i] == '(')
    {
      return str.substr(function_start, i - function_start) + "()";
    }
  }

  return str.substr(function_start);
}

Logger::Logger() :
  m_log_level(kWarning)
{}

void
Logger::incr_log_level(LogLevel level)
{
  if (get_log_level() < level)
  {
    set_log_level(level);
  }
}

void
Logger::set_log_level(LogLevel level)
{
  m_log_level = level;
}

Logger::LogLevel
Logger::get_log_level() const
{
  return m_log_level;
}

void
Logger::append(LogLevel level, 
               const std::string& file, int line,
               const std::string& msg)
{
  switch(level)
  {
    case kError:   std::cerr << "[ERROR "; break;
    case kWarning: std::cerr << "[WARN "; break;
    case kInfo:    std::cerr << "[INFO "; break;
    case kDebug:   std::cerr << "[DEBUG "; break;
    case kTemp:    std::cerr << "[TEMP "; break;
  }

  std::cerr << file << ":" << line << "] " << msg << std::endl;
}

/* EOF */
