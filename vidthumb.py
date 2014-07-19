#!/usr/bin/env python3

import cairo
import datetime
import gi
import sys
import PIL.Image

gi.require_version('Gst', '1.0')

from gi.repository import GObject
from gi.repository import Gst

print("GStreamer Version %s" % (Gst.version(),))

GObject.threads_init()
Gst.init(None)

d = Gst.parse_launch("filesrc name=input ! " +
                     "decodebin ! " +
                     "videoconvert ! " +
                     "video/x-raw,format=RGB ! " + 
                     "fakesink name=output signal-handoffs=True")
source = d.get_by_name("input")

def on_preroll_handoff(element, buf, pad):
    print("preroll-handoff")
    data = buf.extract_dup(0, buf.get_size())
    # print(data)    
    # img = cairo.ImageSurface.create_for_data(data, cairo.FORMAT_RGB24, 320, 200)
    # img = cairo.ImageSurface(cairo.FORMAT_RGB24, 320, 200)
    # print(dir(img.get_data()))
    print(buf.get_sizes())
    structure = pad.get_current_caps().get_structure(0)
    width = structure.get_int("width")[1]
    height = structure.get_int("height")[1]
    print("%dx%d" % (width, height))
    img = PIL.Image.frombuffer("RGB", (width, height), data, "raw", "RGB", 0, 1)
    img.save("/tmp/out.png")

def on_handoff(element, buf, pad):
    print("handoff")
    data = buf.extract_dup(0, buf.get_size())
    # print(data)

def on_message(element, *args):
    print(args)

def on_timeout(*args):
    print("on_timeout: %s" % (args,))

    fmt = Gst.Format(Gst.Format.TIME)
    duration = d.query_duration(fmt)[1]
    print("Pos: %s\n" % (d.query_position(Gst.Format.TIME),))
    print("Duration: %s\n" % duration)

    print(d.get_state(0))

    return True

source.set_property("location", sys.argv[1])

fakesink = d.get_by_name("output")
# fakesink.connect("handoff", on_handoff)
fakesink.connect("preroll-handoff", on_preroll_handoff)
d.get_bus().connect("message", on_message)

d.set_state(Gst.State.PLAYING)

# GObject.timeout_add(100, on_timeout)
GObject.MainLoop().run()

# EOF #
