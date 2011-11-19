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

#ifndef HEADER_PARAM_LIST_HPP
#define HEADER_PARAM_LIST_HPP

#include <map>
#include <string>

class ParamList
{
private:
  std::map<std::string, std::string> m_params;

public:
  ParamList();
  ParamList(const std::string& str);

  void parse_string(const std::string& str);

  bool get(const std::string& name, int* value);
  bool get(const std::string& name, double* value);
  bool get(const std::string& name, bool* value);
  bool get(const std::string& name, std::string* value);

private:
  ParamList(const ParamList&);
  ParamList& operator=(const ParamList&);
};

#endif

/* EOF */
