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

#include "fourd_thumbnailer.hpp"

#include <gstreamermm.h>

FourdThumbnailer::FourdThumbnailer(int slices) :
  m_buffer(),
  m_slices(slices),
  m_count(0)
{
}

std::vector<gint64>
FourdThumbnailer::get_thumbnail_pos(gint64 duration)
{
  std::vector<gint64> lst;
  for(int i = 0; i < m_slices; ++i)
  {
    lst.push_back(i * duration / m_slices);
  }
  return lst;
}

void
FourdThumbnailer::receive_frame(Cairo::RefPtr<Cairo::ImageSurface> img, gint64 pos)
{
  if (!m_buffer)
  {
    m_buffer = Cairo::ImageSurface::create(Cairo::FORMAT_RGB24, 
                                           img->get_width() * 10,
                                           img->get_height());
  }

  Cairo::RefPtr<Cairo::Context> cr = Cairo::Context::create(m_buffer);

  int slice_width = m_buffer->get_width() / m_slices;
  cr->rectangle((slice_width * m_count), 0, 
                slice_width, m_buffer->get_height());
  cr->clip();

  cr->begin_new_path();
  cr->set_source(img, m_count * (m_buffer->get_width() - img->get_width()) / (m_slices-1), 0);
  cr->paint();

  m_count += 1;
}

void
FourdThumbnailer::save(const std::string& filename)
{
  if (m_buffer)
  {
    m_buffer->write_to_png(filename);
  }
}

/* EOF */
