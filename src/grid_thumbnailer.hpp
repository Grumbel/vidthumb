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

#ifndef HEADER_GRID_THUMBNAILER_HPP
#define HEADER_GRID_THUMBNAILER_HPP

#include "thumbnailer.hpp"

class GridThumbnailer : public Thumbnailer
{
private:
  Cairo::RefPtr<Cairo::ImageSurface> m_buffer;
  int m_cols;
  int m_rows;
  int m_image_count;

public:
  GridThumbnailer(int cols, int rows);

  void save(const std::string& filename) override;
  std::vector<gint64> get_thumbnail_pos(gint64 duration) override;
  void receive_frame(Cairo::RefPtr<Cairo::ImageSurface> img, gint64 pos) override;
};

#endif

/* EOF */
