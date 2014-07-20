#ifndef HEADER_MEDIAINFO_HPP
#define HEADER_MEDIAINFO_HPP

#include <gst/gst.h>

class MediaInfo
{
private:
  GMainLoop* m_mainloop;

  GstPipeline* m_pipeline;
  GstElement*  m_playbin;
  GstElement*  m_fakesink;

  gint64 m_duration;
  gint64 m_frames;
  gint64 m_buffers;
  gint64 m_bytes;

  int m_width;
  int m_height;

public:
  MediaInfo(const std::string& filename);
  ~MediaInfo();

  gint64 get_duration() const { return m_duration; }
  gint64 get_frames() const { return m_frames; }
  gint64 get_bytes() const { return m_bytes; }
  gint64 get_buffers() const { return m_buffers; }
  int get_width() const { return m_width; }
  int get_height() const { return m_height; }

  //bool on_buffer_probe(GstPad* pad, const GstMiniObject* miniobj);
  
  //void on_bus_message(const Glib::RefPtr<Gst::Message>& msg);
  void get_information();
  bool shutdown();

private:
  MediaInfo(const MediaInfo&) = delete;
  MediaInfo& operator=(const MediaInfo&) = delete;
};

#endif

/* EOF */
