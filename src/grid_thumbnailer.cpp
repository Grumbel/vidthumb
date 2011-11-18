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

#include "grid_thumbnailer.hpp"

#include <iostream>

GridThumbnailer::GridThumbnailer(int cols, int rows) :
  m_buffer(),
  m_cols(cols),
  m_rows(rows),
  m_image_count(0)
{
}

void
GridThumbnailer::save(const std::string& filename)
{
  m_buffer->write_to_png(filename);
}
  
std::vector<gint64>
GridThumbnailer::get_thumbnail_pos(gint64 duration)
{
  std::cout << "Duration: " << duration << std::endl;
    
  int n = m_rows * m_cols;
  std::vector<gint64> lst;
  for(int i = 0; i < n; ++i)
  {
    lst.push_back(duration/n/2 + duration/n * i);
  }
  return lst;
}

void
GridThumbnailer::receive_frame(Cairo::RefPtr<Cairo::ImageSurface> img) 
{
  std::cout << "Receive_Frame: " << std::endl;
  if (!m_buffer)
  {
    m_buffer = Cairo::ImageSurface::create(Cairo::FORMAT_RGB24, 
                                           img->get_width()  * m_cols, 
                                           img->get_height() * m_rows);
  }

  Cairo::RefPtr<Cairo::Context> cr = Cairo::Context::create(m_buffer);
  cr->set_source(img, 
                 (m_image_count % m_cols) * img->get_width(),
                 (m_image_count / m_cols) * img->get_height());   
  cr->paint();

  m_image_count += 1;
}

/* EOF */
