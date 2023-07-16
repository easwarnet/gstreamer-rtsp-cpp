#include <string>
#include <iostream>
#include <memory>
#include <gst/gst.h>
#include <glib-unix.h>
#include <gst/rtsp-server/rtsp-server.h>

using namespace std;

class RtspServer
{
public:
  RtspServer(string pipeline);
  void RtspServerInit(bool setcallback, const gchar *mountpoint, char *port);
  void RtspServerAddUser(string user, string password, bool access_perm, bool construct_perm);
  int RtspStart();
  void RtspStop();
  ~RtspServer();

private:
  static gboolean RemoveFunc(GstRTSPSessionPool * pool, GstRTSPSession * session, GstRTSPServer * server);
  static gboolean RemoveSessions(GstRTSPServer * server);
  static gboolean PoolCleanup(GstRTSPServer * server);
  static gboolean SignalHandler(gpointer user_data);
  static void OnSsrcActive(GObject * session, GObject * source, GstRTSPMedia * media);
  static void OnSenderSsrcActive(GObject * session, GObject * source, GstRTSPMedia * media);
  static void MediaPreparedCb(GstRTSPMedia * media);
  static void MediaConfigureCb(GstRTSPMediaFactory * factory, GstRTSPMedia * media);
  
  GMainLoop *loop;
  GstRTSPServer *server;
  GstRTSPMountPoints *mounts;
  GstRTSPMediaFactory *factory;
  GError *error = NULL;
  gchar *m_pipeline;
};


