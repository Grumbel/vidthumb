#ifndef HEADER_MEDIAINFO_HPP
#define HEADER_MEDIAINFO_HPP

#include <gstreamermm.h>
#include <glibmm/main.h>

class MediaInfo
{
public:
  MediaInfo(std::string const& filename);
  ~MediaInfo();

  gint64 get_duration() const { return m_duration; }
  int get_width() const { return m_width; }
  int get_height() const { return m_height; }

  void get_information();
  bool shutdown();

private:
  Glib::RefPtr<Glib::MainLoop> m_mainloop;

  Glib::RefPtr<Gst::Pipeline> m_pipeline;
  Glib::RefPtr<Gst::Element> m_playbin;
  Glib::RefPtr<Gst::Element> m_fakesink;

  gint64 m_duration;

  int m_width;
  int m_height;

private:
  MediaInfo(const MediaInfo&) = delete;
  MediaInfo& operator=(const MediaInfo&) = delete;
};

#endif

/* EOF */
