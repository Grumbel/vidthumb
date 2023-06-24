/*
**  VidThumb - Video Thumbnailer
**  Copyright (C) 2011,2014 Ingo Ruhnke <grumbel@gmx.de>
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

#include "video_processor.hpp"

#include <filesystem>
#include <algorithm>
#include <assert.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <cairomm/cairomm.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <logmich/log.hpp>

#include "thumbnailer.hpp"

std::string to_string(Gst::State state)
{
  switch(state)
  {
    case Gst::STATE_VOID_PENDING:
      return "pending";
    case Gst::STATE_NULL:
      return "null";
    case Gst::STATE_READY:
      return "ready";
    case Gst::STATE_PAUSED:
      return "paused";
    case Gst::STATE_PLAYING:
      return "playing";
    default:
      return "unknown";
  }
}

VideoProcessor::VideoProcessor(Glib::RefPtr<Glib::MainLoop> mainloop,
                               Thumbnailer& thumbnailer) :
  m_mainloop(mainloop),
  m_thumbnailer(thumbnailer),
  m_pipeline(),
  m_fakesink(),
  m_thumbnailer_pos(),
  m_done(false),
  m_running(false),
  m_timeout_id(0),
  m_timeout(-1),
  m_accurate(false),
  m_last_screenshot(),
  m_opts()
{
}

VideoProcessor::~VideoProcessor()
{
  if (m_timeout_id)
  {
    g_source_remove(m_timeout_id);
    m_timeout_id = 0;
  }
}

std::string
VideoProcessor::get_pipeline_desc() const
{
  std::ostringstream pipeline_desc;

  pipeline_desc <<
    "uridecodebin name=mysource "
    "  ! videoscale "
    "  ! videoconvert ";

  // force output format
  pipeline_desc << "  ! video/x-raw,format=BGRx";
  if (m_opts.width)
  {
    pipeline_desc << ",width=" << *m_opts.width;
  }

  if (m_opts.height)
  {
    pipeline_desc << ",height=" << *m_opts.height;
  }

  if (m_opts.keep_aspect_ratio)
  {
    pipeline_desc << ",pixel-aspect-ratio=1/1";
  }

  pipeline_desc << "  ! fakesink name=mysink signal-handoffs=True sync=False ";

  return pipeline_desc.str();
}

void
VideoProcessor::setup_pipeline()
{
  auto pipeline_desc = get_pipeline_desc();
  log_info("Using pipeline: {}", pipeline_desc);
  Glib::RefPtr<Gst::Element> playbin = Gst::Parse::launch(pipeline_desc);
  m_pipeline = m_pipeline.cast_static(playbin);

  m_fakesink = Glib::RefPtr<Gst::FakeSink>::cast_static(m_pipeline->get_element("mysink"));

  Glib::RefPtr<Gst::Bus> thumbnail_bus = m_pipeline->get_bus();

  // intercept the frame data and makes thumbnails
  m_fakesink->signal_preroll_handoff().connect(sigc::mem_fun(*this, &VideoProcessor::on_preroll_handoff));

  // listen to bus messages
  thumbnail_bus->add_watch(sigc::mem_fun(*this, &VideoProcessor::on_bus_message));
}

void
VideoProcessor::set_accurate(bool accurate)
{
  m_accurate = accurate;
}

void
VideoProcessor::set_options(const VideoProcessorOptions opts)
{
  m_opts = opts;
}

void
VideoProcessor::set_timeout(int timeout)
{
  if (m_timeout_id)
  {
    g_source_remove(m_timeout_id);
    m_timeout_id = 0;
  }

  m_timeout = timeout;

  if (m_timeout != -1)
  {
    m_last_screenshot = g_get_real_time();
    log_info("------------------------------------ install time out " );
    auto callback = [](gpointer user_data) -> gboolean
      {
        return static_cast<VideoProcessor*>(user_data)->on_timeout();
      };
    g_timeout_add(m_timeout, callback, this);
  }
}

void
VideoProcessor::open(const std::string& filename)
{
  setup_pipeline();

  Glib::ustring uri = Glib::filename_to_uri(Glib::canonicalize_filename(filename));

  Glib::RefPtr<Gst::Element> source = m_pipeline->get_element("mysource");
  source->set_property("uri", uri);

  // bring stream into pause state so that the thumbnailing can begin
  m_pipeline->set_state(Gst::STATE_PAUSED);
}

gint64
VideoProcessor::get_duration()
{
  Glib::RefPtr<Gst::Query> query_duration = Gst::QueryDuration::create(Gst::FORMAT_TIME);
  if (m_pipeline->query(query_duration))
  {
    gint64 duration = Glib::RefPtr<Gst::QueryDuration>::cast_static(query_duration)->parse();
    return duration;
  }
  else
  {
    throw std::runtime_error("error: QUERY FAILURE");
  }

  return 0;
}

gint64
VideoProcessor::get_position()
{
  Glib::RefPtr<Gst::Query> query_position = Gst::QueryPosition::create(Gst::FORMAT_TIME);
  if (m_pipeline->query(query_position))
  {
    gint64 position = Glib::RefPtr<Gst::QueryPosition>::cast_static(query_position)->parse();
    return position;
  }
  else
  {
    throw std::runtime_error("error: QUERY FAILURE");
  }

  return 0;
}

void
VideoProcessor::seek_step()
{
  log_info("!!!!!!!!!!!!!!!! seek_step: {}", m_thumbnailer_pos.size());

  if (!m_thumbnailer_pos.empty())
  {
    Gst::SeekFlags seek_flags;
    if (m_accurate)
    {
      seek_flags = Gst::SEEK_FLAG_FLUSH | Gst::SEEK_FLAG_ACCURATE;  // slow
    }
    else
    {
      seek_flags = Gst::SEEK_FLAG_FLUSH | Gst::SEEK_FLAG_KEY_UNIT | Gst::SEEK_FLAG_SNAP_NEAREST;  // fast
    }

    log_info("--> REQUEST SEEK: {}", m_thumbnailer_pos.back());
    if (!m_pipeline->seek(Gst::FORMAT_TIME,
                          seek_flags,
                          m_thumbnailer_pos.back()))
    {
      log_info(">>>>>>>>>>>>>>>>>>>> SEEK FAILURE <<<<<<<<<<<<<<<<<<");
    }

    m_thumbnailer_pos.pop_back();
  }
  else
  {
    log_info("---------- DONE ------------");

    queue_shutdown();

    m_done = true;
  }
}

Cairo::RefPtr<Cairo::ImageSurface> buffer2cairo(Glib::RefPtr<Gst::Buffer> const& buffer, Glib::RefPtr<Gst::Pad> const& pad)
{
  Glib::RefPtr<Gst::Caps> caps = pad->get_current_caps();
  Gst::Structure structure = caps->get_structure(0);

  int width, height;
  structure.get_field("width",  width);
  structure.get_field("height", height);

  Cairo::RefPtr<Cairo::ImageSurface> img = Cairo::ImageSurface::create(Cairo::FORMAT_RGB24, width, height);

  unsigned char* op = img->get_data();

  int ostride = img->get_stride();
  int istride = static_cast<int>(buffer->get_size() / height);
  for(int y = 0; y < height; ++y)
  {
    buffer->extract(y * istride,
                    op + y * ostride,
                    width * 4);
  }

  return img;
}

void
VideoProcessor::on_preroll_handoff(Glib::RefPtr<Gst::Buffer> const& buffer,
                                   Glib::RefPtr<Gst::Pad> const& pad)
{
  log_info(">>>>>>>>>>>>>>>>> preroll_handoff: {}", get_position());
  if (m_running)
  {
    m_last_screenshot = g_get_real_time();
    auto img = buffer2cairo(buffer, pad);
    m_thumbnailer.receive_frame(img, get_position());

    Glib::signal_idle().connect([this]() -> gboolean {
      seek_step();
      return false;
    });
  }
}

bool
VideoProcessor::on_bus_message(Glib::RefPtr<Gst::Bus> const& bus,
                               Glib::RefPtr<Gst::Message> const& message)
{
  switch(message->get_message_type())
  {
    case Gst::MESSAGE_ERROR:
      {
        Glib::RefPtr<Gst::MessageError> const msgError = Glib::RefPtr<Gst::MessageError>::cast_static(message);
        Glib::Error const err = msgError->parse_error();

        std::cerr << "Error: " << err.what() << std::endl;
        log_error("MessageError: {}", err.what());

        queue_shutdown();
      }
      break;

    case Gst::MESSAGE_STATE_CHANGED:
      {
        Gst::State oldstate;
        Gst::State newstate;
        Gst::State pending;
        Glib::RefPtr<Gst::MessageStateChanged> const state_changed_msg = Glib::RefPtr<Gst::MessageStateChanged>::cast_static(message);
        state_changed_msg->parse(oldstate, newstate, pending);

        log_debug("Gst::MESSAGE_STATE_CHANGED: name: {} old_state: {}  new_state: {}",
                  state_changed_msg->get_source()->get_name(),
                  Gst::Enums::get_name(state_changed_msg->parse_old_state()),
                  Gst::Enums::get_name(state_changed_msg->parse_new_state()));

        if (state_changed_msg->get_source() == m_fakesink &&
            newstate == Gst::STATE_PAUSED &&
            !m_running)
        {
          log_info("##################################### ONLY ONCE: ################");
          m_thumbnailer_pos = m_thumbnailer.get_thumbnail_pos(get_duration());
          std::reverse(m_thumbnailer_pos.begin(), m_thumbnailer_pos.end());
          m_running = true;
          seek_step();
        }
      }
      break;

    case Gst::MESSAGE_EOS:
      {
        log_debug("GST_MESSAGE_EOS");
        queue_shutdown();
      }
      break;

    case Gst::MESSAGE_TAG:
      {
        log_info("TAG: ");
        auto tag_msg = Glib::RefPtr<Gst::MessageTag>::cast_static(message);
        Gst::TagList tag_list = tag_msg->parse_tag_list();
        tag_list.foreach([&tag_list](Glib::ustring const& foo){
          log_info("  name: {} {}", foo, tag_list.get_type(foo));
        });
      }
      break;

    case Gst::MESSAGE_ASYNC_DONE:
      log_info("------------------------------------- GST_MESSAGE_ASYNC_DONE");
      break;

    case Gst::MESSAGE_STREAM_STATUS:
      {
        log_debug("GST_MESSAGE_STREAM_STATUS: ");
        auto stream_status_msg = Glib::RefPtr<Gst::MessageStreamStatus>::cast_static(message);

        Gst::StreamStatusType const stream_status_type = stream_status_msg->parse_type();
        switch (stream_status_type)
        {
          case Gst::STREAM_STATUS_TYPE_CREATE:
            log_info("CREATE");
            break;

          case Gst::STREAM_STATUS_TYPE_ENTER:
            log_info("ENTER");
            break;

          case Gst::STREAM_STATUS_TYPE_LEAVE:
            log_info("LEAVE");
            break;

          case Gst::STREAM_STATUS_TYPE_DESTROY:
            log_info("DESTROY");
            break;

          case Gst::STREAM_STATUS_TYPE_START:
            log_info("START");
            break;

          case Gst::STREAM_STATUS_TYPE_PAUSE:
            log_info("PAUSE");
            break;

          case Gst::STREAM_STATUS_TYPE_STOP:
            log_info("PAUSE");
            break;

          default:
            log_info("???");
            break;
        }
      }
      break;

    default:
      log_debug("unhandled message: {}", message->get_message_type());
      break;
  }

  return true;
}

void
VideoProcessor::queue_shutdown()
{
  Glib::signal_idle().connect([this]() -> gboolean {
    shutdown();
    return false;
  });
}

void
VideoProcessor::shutdown()
{
  log_info("Going to shutdown!!!!!!!!!!!");
  m_pipeline->set_state(Gst::STATE_NULL);
  m_mainloop->quit();
}

bool
VideoProcessor::on_timeout()
{
  guint64 t = g_get_real_time();

  t = t - m_last_screenshot;

  double t_d = static_cast<double>(t) / G_USEC_PER_SEC;

  log_info("TIMEOUT: {} {}", t_d, m_timeout);
  if (t_d > m_timeout/1000.0)
  {
    log_info("--------- timeout ----------------: {}", t_d);
    queue_shutdown();
  }

  return true;
}

/* EOF */
