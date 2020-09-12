/*
**  VidThumb - Video Thumbnailer
**  Copyright (C) 2011 Ingo Ruhnke <grumbel@gmx.de>
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

#include "param_list.hpp"

#include <string>
#include <vector>

namespace {

void split_string_at(const std::string& str, char c, std::string* lhs, std::string* rhs)
{
  std::string::size_type p = str.find(c);
  if (p == std::string::npos)
  {
    *lhs = str;
  }
  else
  {
    *lhs = str.substr(0, p);
    *rhs = str.substr(p+1);
  }
}

std::vector<std::string> string_split(std::string_view text, char delimiter)
{
  std::vector<std::string> result;

  std::string::size_type start = 0;
  for(std::string::size_type i = 0; i != text.size(); ++i)
  {
    if (text[i] == delimiter)
    {
      result.emplace_back(text.substr(start, i - start));
      start = i + 1;
    }
  }

  result.emplace_back(text.substr(start));

  return result;
}

} // namespace

ParamList::ParamList() :
  m_params()
{
}

ParamList::ParamList(const std::string& str) :
  m_params()
{
  parse_string(str);
}

void
ParamList::parse_string(const std::string& str)
{
  for(auto const& token : string_split(str, ','))
  {
    std::string lhs;
    std::string rhs;

    split_string_at(token, '=', &lhs, &rhs);

    m_params[lhs] = rhs;
  }
}

bool
ParamList::get(const std::string& name, int* value)
{
  auto it = m_params.find(name);
  if (it != m_params.end())
  {
    *value = std::stoi(it->second);
    return true;
  }
  else
  {
    return false;
  }
}

bool
ParamList::get(const std::string& name, double* value)
{
  auto it = m_params.find(name);
  if (it != m_params.end())
  {
    *value = std::stod(it->second);
    return true;
  }
  else
  {
    return false;
  }
}

bool
ParamList::get(const std::string& name, bool* value)
{
  auto it = m_params.find(name);
  if (it != m_params.end())
  {
    *value = static_cast<bool>(std::stoi(it->second));
    return true;
  }
  else
  {
    return false;
  }
}

bool
ParamList::get(const std::string& name, std::string* value)
{
  auto it = m_params.find(name);
  if (it != m_params.end())
  {
    *value = it->second;
    return true;
  }
  else
  {
    return false;
  }
}

#ifdef __TEST__

int main(int argc, char** argv)
{
  for(int i = 1; i < argc; ++i)
  {
    ParamList params(argv[i]);

    {
      std::string value = "<empty>";
      params.get("width", &value);
      std::cout << value << std::endl;
    }

    {
      std::string value = "<empty>";
      params.get("height", &value);
      std::cout << value << std::endl;
    }

    {
      std::string value = "<empty>";
      params.get("size", &value);
      std::cout << value << std::endl;
    }

    {
      bool value = false;
      params.get("bool", &value);
      std::cout << value << std::endl;
    }
  }
  return 0;
}

#endif

/* EOF */
