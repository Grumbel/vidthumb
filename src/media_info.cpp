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

#include <glibmm.h>

#include <logmich/log.hpp>

#include "media_info.hpp"

MediaInfo::MediaInfo(std::string const& filename) :
  m_mainloop(),
  m_pipeline(),
  m_playbin(),
  m_fakesink(),
  m_duration(),
  m_width(),
  m_height()
{
  m_playbin = Gst::Parse::launch("uridecodebin name=mysource ! fakesink name=mysink");
  m_pipeline = m_pipeline.cast_static(m_playbin);

  Glib::RefPtr<Gst::Element> source = m_pipeline->get_element("mysource");
  Glib::ustring uri = Glib::filename_to_uri(Glib::canonicalize_filename(filename));
  source->set_property("uri", uri);

  m_fakesink = m_pipeline->get_element("mysink");

  m_mainloop = Glib::MainLoop::create(false);

  Glib::RefPtr<Gst::Bus> bus = m_pipeline->get_bus();
  bus->add_watch([this](Glib::RefPtr<Gst::Bus> const& bus_,
                        Glib::RefPtr<Gst::Message> const& message) -> bool
  {
    switch(message->get_message_type()) {
      case Gst::MESSAGE_EOS:
        log_info("-------- End of stream");
        m_mainloop->quit();
        return false;

      case Gst::MESSAGE_ERROR:
        {
          Glib::RefPtr<Gst::MessageError> msgError = Glib::RefPtr<Gst::MessageError>::cast_static(message);
          if(msgError)
          {
            Glib::Error err = msgError->parse_error();
            std::cerr << "Error: " << err.what() << std::endl;
            log_error("MessageError: {}", err.what());
          }
          else
          {
            log_error("MessageError: unknown");
          }

          m_pipeline->set_state(Gst::STATE_NULL);
          m_mainloop->quit();
          return false;
        }
        break;

      case Gst::MESSAGE_TAG:
        {
          log_info("TAG: ");
          auto tag_msg = Glib::RefPtr<Gst::MessageTag>::cast_static(message);
          Gst::TagList tag_list = tag_msg->parse_tag_list();
          // log_info("  is_empty: {}", tag_list.is_empty());
          tag_list.foreach([&tag_list](Glib::ustring const& foo){
            log_info("  name: {} {}", foo, tag_list.get_type(foo));
          });
        }
        break;

      case Gst::MESSAGE_ASYNC_DONE:
        // triggered when PAUSE state reached
        log_debug("----- Gst::MESSAGE_ASYNC_DONE");

        get_information();
        m_pipeline->set_state(Gst::STATE_NULL);
        m_mainloop->quit();
        break;

      case Gst::MESSAGE_STATE_CHANGED:
        {
        Glib::RefPtr<Gst::MessageStateChanged> state_changed_msg = Glib::RefPtr<Gst::MessageStateChanged>::cast_static(message);
        Gst::State oldstate;
        Gst::State newstate;
        Gst::State pending;
        state_changed_msg->parse(oldstate, newstate, pending);

        log_debug("Gst::MESSAGE_STATE_CHANGED: old_state: {}  new_state: {}",
                  Gst::Enums::get_name(state_changed_msg->parse_old_state()),
                  Gst::Enums::get_name(state_changed_msg->parse_new_state()));

        /*
        if (msg->get_source() == m_pipeline && newstate == Gst::STATE_PAUSED)
        {
          get_information();
          Glib::signal_idle().connect(sigc::mem_fun(this, &MediaInfo::shutdown));
        }
        */
        }
        break;

      case Gst::MESSAGE_STREAM_START:
        log_debug("Gst::MESSAGE_STREAM_START");
        break;

      case Gst::MESSAGE_STREAM_STATUS:
        {
          //auto message_stream_status = message->parse_stream_status();
          auto message_stream_status = Glib::RefPtr<Gst::MessageStreamStatus>::cast_static(message);
          log_debug("Gst::MESSAGE_STREAM_STATUS: {}",
                    message_stream_status->parse_type());
        }
        break;

      case Gst::MESSAGE_DURATION_CHANGED:
        log_debug("Gst::MESSAGE_DURATION_CHANGED");
        break;

      case Gst::MESSAGE_LATENCY:
        log_debug("Gst::MESSAGE_LATENCY");
        break;

      case Gst::MESSAGE_QOS:
        log_debug("Gst::MESSAGE_QOS");
        break;

      default:
        log_error("--------- unhandled message: {}", Gst::Enums::get_name(message->get_message_type()));
        break;
    }

    return true;
  });

  m_playbin->set_state(Gst::STATE_PAUSED);
  m_mainloop->run();
}

MediaInfo::~MediaInfo()
{
}

void
MediaInfo::get_information()
{
  Glib::RefPtr<Gst::Query> query_duration = Gst::QueryDuration::create(Gst::FORMAT_TIME);
  if (m_playbin->query(query_duration)) {
    gint64 duration = Glib::RefPtr<Gst::QueryDuration>::cast_static(query_duration)->parse();
    log_info("Duration: {}sec", static_cast<double>(duration) / 1000.0 / 1000.0 / 1000.0);
  }

  Glib::RefPtr<Gst::Pad> mypad = m_fakesink->get_static_pad("sink");
  Glib::RefPtr<Gst::Caps> caps = mypad->get_current_caps();
  Gst::Structure structure = caps->get_structure(0);

  if (structure)
  {
    structure.foreach([&structure](const Glib::ustring& key, const Glib::ValueBase& value) {
      log_info("   -- field: {} {}", key, structure.get_field_type(key));
      return true; // continue looping
    });

    structure.get_field("width",  m_width);
    structure.get_field("height", m_height);

    Gst::Fraction m_aspect_ratio;
    if (structure.get_field("pixel-aspect-ratio", m_aspect_ratio))
    {
      log_info("  ## PixelAspectRatio: '{}/{}'", m_aspect_ratio.num, m_aspect_ratio.denom);
    }

    Gst::Fraction m_framerate;
    if (structure.get_field("framerate", m_framerate))
    {
      log_info("  ## Framerate: {}/{}", m_framerate.num, m_framerate.denom);
    }

    std::string m_interlaced;
    if (structure.get_field("interlace-mode", m_interlaced))
    {
      log_info("  ## Interlaced: {}", m_interlaced);
    }

    std::string m_format;
    if (structure.get_field("format", m_format))
    {
      log_info("  ## Format: {}", m_format);
    }

    log_info("  ## WidthxHeight: {}x{}", m_width, m_height);
  }
}

bool
MediaInfo::shutdown()
{
  log_info("Going to shutdown!!!!!!!!!!!");

  m_playbin->set_state(Gst::STATE_NULL);
  m_mainloop->quit();

  return false;
}

int main(int argc, char** argv)
{
  logmich::incr_log_level(logmich::LogLevel::INFO);

  Glib::init();
  Gst::init(argc, argv);

  try {
    for(int i = 1; i < argc; ++i)
    {
      try
      {
        log_info("--- processing {}", argv[i]);

        MediaInfo video(argv[i]);
        log_info("Duration: {} - {}:{}:{}",
                 video.get_duration(),
                 static_cast<int>(video.get_duration() / (GST_SECOND * 60 * 60)),
                 static_cast<int>(video.get_duration() / (GST_SECOND * 60)) % 60,
                 static_cast<int>(video.get_duration() / GST_SECOND) % 60);
        log_info("Size:     {}x{}", video.get_width(), video.get_height());
      }
      catch(const std::exception& err)
      {
        log_info("Exception: ", err.what());
      }
    }
  } catch (std::exception const& err) {
    std::cerr << "error: " << err.what() << std::endl;
  } catch (Glib::Error const& err) {
    std::cerr << "error: code " << err.code() << ": " << err.what() << std::endl;
  }

  Gst::deinit();

  return 0;
}

/* EOF */
