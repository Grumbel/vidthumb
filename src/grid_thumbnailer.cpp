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

#include <boost/format.hpp>
#include <gst/gst.h>
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
  if (m_buffer)
  {
    m_buffer->write_to_png(filename);
  }
}
  
std::vector<gint64>
GridThumbnailer::get_thumbnail_pos(gint64 duration)
{
  int n = m_rows * m_cols;
  std::vector<gint64> lst;
  for(int i = 0; i < n; ++i)
  {
    lst.push_back(duration/n/2 + duration/n * i);
  }
  return lst;
}

void
GridThumbnailer::receive_frame(Cairo::RefPtr<Cairo::ImageSurface> img, gint64 pos) 
{
  if (!m_buffer)
  {
    m_buffer = Cairo::ImageSurface::create(Cairo::FORMAT_RGB24, 
                                           img->get_width()  * m_cols, 
                                           img->get_height() * m_rows);
  }

  int x = (m_image_count % m_cols) * img->get_width();
  int y = (m_image_count / m_cols) * img->get_height();

  Cairo::RefPtr<Cairo::Context> cr = Cairo::Context::create(m_buffer);
  cr->set_source(img, x, y);
  cr->paint();

  cr->set_font_size(12.0);
  cr->select_font_face ("Sans", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL); 
  Cairo::FontOptions font_options;
  font_options.set_hint_metrics (Cairo::HINT_METRICS_ON);
  font_options.set_hint_style(Cairo::HINT_STYLE_FULL);
  font_options.set_antialias(Cairo::ANTIALIAS_GRAY);
  cr->set_font_options(font_options);

  int hour = static_cast<int>(pos / (GST_SECOND * 60 * 60));
  int min  = static_cast<int>(pos / (GST_SECOND * 60)) % 60;
  int sec  = static_cast<int>(pos / GST_SECOND) % 60;
  std::string time_str = (boost::format("%02d:%02d:%02d") % hour % min % sec).str();

  int outline = 1;
  cr->set_source_rgb(0,0,0);
  for(int iy = -outline; iy <= outline; ++iy)
  {
    for(int ix = -outline; ix <= outline; ++ix)
    {
      cr->move_to(x+6 + ix, y+14 + iy);
      cr->show_text(time_str);
    }
  }
  cr->set_source_rgb(1,1,1);
  cr->move_to(x+6, y+14);
  cr->show_text(time_str);

  m_image_count += 1;
}

/* EOF */
