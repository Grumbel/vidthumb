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

#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>

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
  typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
  tokenizer tokens(str, boost::char_separator<char>(",", ""));

  for(tokenizer::iterator i = tokens.begin(); i != tokens.end(); ++i)
  {
    std::string lhs;
    std::string rhs;

    split_string_at(*i, '=', &lhs, &rhs);

    m_params[lhs] = rhs;
  }
}

bool
ParamList::get(const std::string& name, int* value)
{
  auto it = m_params.find(name);
  if (it != m_params.end())
  {
    *value = boost::lexical_cast<int>(it->second);
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
    *value = boost::lexical_cast<double>(it->second);
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
    *value = boost::lexical_cast<bool>(it->second);
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
