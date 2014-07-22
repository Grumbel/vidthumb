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

#include <iostream>
#include <typeinfo> 
#include <glib.h>
#include <log.hpp>

#include "media_info.hpp"

MediaInfo::MediaInfo(const std::string& filename) :
  m_mainloop(),
  m_pipeline(),
  m_playbin(),
  m_fakesink(),
  m_duration(),
  m_frames(),
  m_buffers(),
  m_bytes(),
  m_width(),
  m_height()
{
  m_playbin = gst_parse_launch("uridecodebin name=mysource ! fakesink name=mysink", NULL);
  m_pipeline = GST_PIPELINE(m_playbin);

  GstElement* source = gst_bin_get_by_name(GST_BIN(m_pipeline), "mysource");
  gchar* uri = g_filename_to_uri(filename.c_str(), NULL, NULL);
  g_object_set(source, "uri", uri, NULL);
  g_free(uri);

  m_fakesink = gst_bin_get_by_name(GST_BIN(m_pipeline), "mysink");

  GstBus* bus = gst_pipeline_get_bus(m_pipeline);
  gst_bus_add_signal_watch(bus);
  //bus->signal_message().connect(sigc::mem_fun(this, &MediaInfo::on_bus_message));

  gst_element_set_state(GST_ELEMENT(m_pipeline), GST_STATE_PAUSED);

  m_mainloop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(m_mainloop);
}

MediaInfo::~MediaInfo()
{
  // FIXME: free some garbage
  //gst_object_unref(bus);
  //gst_element_set_state(pipeline, GST_STATE_NULL);
  //gst_object_unref(m_pipeline);
}

#if 0
struct ForEach
{
  Gst::TagList m_tag_list;

  ForEach(Gst::TagList& tag_list) :
    m_tag_list(tag_list)
  {
  }

  void operator()(const Glib::ustring& foo)
  {
    log_info("  name: " << foo << " " << m_tag_list.get_type(foo)
             );
  }
};
#endif

#if 0
void
MediaInfo::on_bus_message(const Glib::RefPtr<Gst::Message>& msg)
{
  //log_info("MediaInfo::on_bus_message");

  if (msg->get_message_type() & Gst::MESSAGE_ERROR)
  {
    Glib::RefPtr<Gst::MessageError> error_msg = Glib::RefPtr<Gst::MessageError>::cast_dynamic(msg);
    log_info("Error: "
              << msg->get_source()->get_name() << ": "
              << error_msg->parse().what());
    //assert(!"Failure");
    Glib::signal_idle().connect(sigc::mem_fun(this, &MediaInfo::shutdown));
  }
  else if (msg->get_message_type() & Gst::MESSAGE_STATE_CHANGED)
  {
    Glib::RefPtr<Gst::MessageStateChanged> state_msg = Glib::RefPtr<Gst::MessageStateChanged>::cast_dynamic(msg);
    Gst::State oldstate;
    Gst::State newstate;
    Gst::State pending;
    state_msg->parse(oldstate, newstate, pending);

    //log_info("-- message: " << msg->get_source()->get_name() << " " << oldstate << " " << newstate);

    if (msg->get_source() == m_pipeline && newstate == Gst::STATE_PAUSED)
    {
      get_information();
      Glib::signal_idle().connect(sigc::mem_fun(this, &MediaInfo::shutdown));
    }
  }
  else if (msg->get_message_type() & Gst::MESSAGE_TAG)
  {
    if (true)
    {
      log_info("TAG: ");
      auto tag_msg = Glib::RefPtr<Gst::MessageTag>::cast_dynamic(msg);
      log_info("<<<<<<<<<<<<<<: " << tag_msg);
      Gst::TagList tag_list = tag_msg->parse();
      log_info("  is_empty: " << tag_list.is_empty());
      tag_list.foreach(ForEach(tag_list));
      log_info(">>>>>>>>>>>>>>");
      if (false) // does not work
      {
        Glib::RefPtr<Gst::Pad> pad = tag_msg->parse_pad();
        log_info(" PAD: " << pad);
      }
    }
  }
}

bool do_foreach(const Glib::ustring& str, const Glib::ValueBase& value)
{
  //auto& v = reinterpret_cast<const Glib::Value<int>& >(value);
  log_info("   -- field: " << str << " " << typeid(&value).name() << " "
           );
  return true; // continue looping
}
#endif

void
MediaInfo::get_information()
{
#if 0
  GstFormat time_format;

  // does not work 
  time_format = GST_FORMAT_DEFAULT;
  m_playbin->query_duration(time_format, m_frames);

  // does not work
  time_format = GST_FORMAT_BYTES;
  m_playbin->query_duration(time_format, m_bytes);

  // works
  time_format = GST_FORMAT_TIME;
  m_playbin->query_duration(time_format, m_duration);

  GstPad* mypad = gst_element_get_static_pad(m_fakesink, "sink");
  GstCaps* caps = gst_pad_get_current_caps(mypad);
  GstStructure* structure = gst_caps_get_structure(caps, 0);

  if (structure)
  {
    structure.foreach(&do_foreach);

    structure.get_field("width",  m_width);
    structure.get_field("height", m_height);
    
    //*       "framerate", G_TYPE_DOUBLE, 25.0,
    //*       "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,

    //-- field: framerate 
    //-- field: format 
    //-- field: interlaced 
    //-- field: pixel-aspect-ratio 

    Gst::Fraction m_aspect_ratio;
    if (structure.get_field("pixel-aspect-ratio", m_aspect_ratio))
    {
      log_info("  ## PixelAspectRatio: '" << m_aspect_ratio.num << "/" << m_aspect_ratio.denom << "'");
    }

    Gst::Fraction m_framerate;
    if (structure.get_field("framerate", m_framerate))
    {
      log_info("  ## Framerate: " << m_framerate.num << "/" << m_framerate.denom);
    }

    bool m_interlaced;
    if (structure.get_field("interlaced", m_interlaced))
    {
      log_info("  ## Interlaced: " << m_interlaced);
    }

    Gst::Fourcc m_format;
    if (structure.get_field("format", m_format))
    {
      log_info("  ## Format: " << m_format.get_fourcc() << " "
                << m_format.first << " "
                << m_format.second << " "
                << m_format.third << " "
                << m_format.fourth << " "
               );
    }

    log_info("  ## WidthxHeight: " << m_width << "x" << m_height);
  }
#endif
}

bool
MediaInfo::shutdown()
{
  log_info("Going to shutdown!!!!!!!!!!!");
  gst_element_set_state(m_playbin, GST_STATE_NULL);
  g_main_loop_quit(m_mainloop);
  return false;
}

int main(int argc, char** argv)
{
  gst_init(&argc, &argv);

  for(int i = 1; i < argc; ++i)
  {
    try 
    {
      log_info("----------------------------------------------------------------------------------");
      MediaInfo video(argv[i]);
      log_info("Duration: %s - %s:%s:%s",
               video.get_duration(),
               static_cast<int>(video.get_duration() / (GST_SECOND * 60 * 60)),
               static_cast<int>(video.get_duration() / (GST_SECOND * 60)) % 60,
               static_cast<int>(video.get_duration() / GST_SECOND) % 60);
      log_info("Frames:   %s", video.get_frames());
      log_info("Bytes:    %s", video.get_bytes());
      log_info("Buffers:  %s", video.get_buffers());
      log_info("Size:     %dx%d", video.get_width(), video.get_height());
    }
    catch(const std::exception& err)
    {
      log_info("Exception: ", err.what());
    }
  }
  gst_deinit();
  
  return 0;
}

/* EOF */
