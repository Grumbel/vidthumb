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

#ifndef HEADER_FOURD_THUMBNAILER_HPP
#define HEADER_FOURD_THUMBNAILER_HPP

#include "thumbnailer.hpp"

class FourdThumbnailer : public Thumbnailer
{
private:
  Cairo::RefPtr<Cairo::ImageSurface> m_buffer;
  int m_slices;
  int m_count;

public:
  FourdThumbnailer(int slices);

  std::vector<gint64> get_thumbnail_pos(gint64 duration) override;
  void receive_frame(Cairo::RefPtr<Cairo::ImageSurface> img, gint64 pos) override;
  void save(const std::string& filename) override;

private:
  FourdThumbnailer(const FourdThumbnailer&);
  FourdThumbnailer& operator=(const FourdThumbnailer&);
};

#endif

/* EOF */
