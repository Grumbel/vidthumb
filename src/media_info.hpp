#ifndef HEADER_MEDIAINFO_HPP
#define HEADER_MEDIAINFO_HPP

#include <gstreamermm.h>

class MediaInfo
{
private:
  Glib::RefPtr<Glib::MainLoop> m_mainloop;

  Glib::RefPtr<Gst::Pipeline> m_pipeline;
  Glib::RefPtr<Gst::Element>  m_playbin;
  Glib::RefPtr<Gst::Element>  m_fakesink;

  gint64 m_duration;
  gint64 m_frames;
  gint64 m_buffers;
  gint64 m_bytes;

  int m_width;
  int m_height;

public:
  MediaInfo(const std::string& filename);

  gint64 get_duration() const { return m_duration; }
  gint64 get_frames() const { return m_frames; }
  gint64 get_bytes() const { return m_bytes; }
  gint64 get_buffers() const { return m_buffers; }
  int get_width() const { return m_width; }
  int get_height() const { return m_height; }

  bool on_buffer_probe(const Glib::RefPtr<Gst::Pad>& pad, const Glib::RefPtr<Gst::MiniObject>& miniobj);
  
  void on_bus_message(const Glib::RefPtr<Gst::Message>& msg);
  void get_information();
  bool shutdown();
};

#endif

/* EOF */
