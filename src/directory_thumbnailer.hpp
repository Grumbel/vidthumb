/*
**  VidThumb - Video Thumbnailer
**  Copyright (C) 2014 Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef HEADER_DIRECTORY_THUMBNAILER_HPP
#define HEADER_DIRECTORY_THUMBNAILER_HPP

#include "thumbnailer.hpp"

#include <vector>

class DirectoryThumbnailer final : public Thumbnailer
{
private:
  struct Thumbnail
  {
    Cairo::RefPtr<Cairo::ImageSurface> image;
    gint64 pos;
  };

private:
  int m_num;
  std::vector<Thumbnail> m_thumbnails;

public:
  DirectoryThumbnailer(int num);

  std::vector<gint64> get_thumbnail_pos(gint64 duration) override;
  void receive_frame(Cairo::RefPtr<Cairo::ImageSurface> img, gint64 pos) override;
  void save(const std::string& filename) override;

private:
  DirectoryThumbnailer(const DirectoryThumbnailer&) = delete;
  DirectoryThumbnailer& operator=(const DirectoryThumbnailer&) = delete;
};

#endif

/* EOF */
