#include <string>
#include <iostream>
#include <memory>
#include "Gstreamer.h"

#define DEFAULT_RTSP_PORT "8554"

static char *port = (char *) DEFAULT_RTSP_PORT;

int main(int argc, char *argv[]) {
  string pipeline = " ( v4l2src ! videoscale ! "
                        "capsfilter caps=video/x-raw ! videoconvert ! clockoverlay ! "
                        "queue ! x264enc ! rtph264pay name=pay0 pt=96 ) ";
  RtspServer rtspServer(pipeline);
  rtspServer.RtspServerInit(false, "/test", port);
  rtspServer.RtspServerAddUser("admin", "password", true, true);
  rtspServer.RtspStart();
}
