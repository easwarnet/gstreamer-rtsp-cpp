#include <string>
#include <iostream>
#include <memory>
#include "Gstreamer.h"

#define DEFAULT_RTSP_PORT "8554"

static char *port = (char *) DEFAULT_RTSP_PORT;


gboolean RtspServer::SignalHandler(gpointer user_data)
{
  GMainLoop *loop = (GMainLoop *) user_data;
  cout << "RtspServer: Stopping" << endl;

  //g_print("closing the main loop");
  g_main_loop_quit(loop);

  return TRUE;
}

gboolean RtspServer::RemoveFunc(GstRTSPSessionPool * pool, GstRTSPSession * session, GstRTSPServer * server)
{
  return GST_RTSP_FILTER_REMOVE;
}

gboolean RtspServer::RemoveSessions(GstRTSPServer * server)
{
  GstRTSPSessionPool *pool;

  g_print("RtspServer: Removing all sessions\n");
  pool = gst_rtsp_server_get_session_pool(server);
  gst_rtsp_session_pool_filter(pool,
     (GstRTSPSessionPoolFilterFunc) RemoveFunc, server);
  g_object_unref(pool);

  return FALSE;
}

gboolean RtspServer::PoolCleanup(GstRTSPServer * server)
{
  GstRTSPSessionPool *pool;
  g_print("RtspServer: Pool Cleanup\n");
  pool = gst_rtsp_server_get_session_pool(server);
  gst_rtsp_session_pool_cleanup(pool);
  g_object_unref(pool);

  return TRUE;
}

/* called when a stream has received an RTCP packet from the client */
void RtspServer::OnSsrcActive(GObject * session, GObject * source, GstRTSPMedia * media)
{
  GstStructure *stats;

  GST_INFO("source %p in session %p is active", source, session);

  g_object_get(source, "stats", &stats, NULL);
  if(stats) {
    gchar *sstr;

    sstr = gst_structure_to_string(stats);
    g_print("structure: %s\n", sstr);
    g_free(sstr);

    gst_structure_free(stats);
  }
}

void RtspServer::OnSenderSsrcActive(GObject * session, GObject * source, GstRTSPMedia * media)
{
  GstStructure *stats;

  GST_INFO("source %p in session %p is active", source, session);

  g_object_get(source, "stats", &stats, NULL);
  if(stats) {
    gchar *sstr;

    sstr = gst_structure_to_string(stats);
    g_print("Sender stats:\nstructure: %s\n", sstr);
    g_free(sstr);

    gst_structure_free(stats);
  }
}

/* signal callback when the media is prepared for streaming. We can get the
 * session manager for each of the streams and connect to some signals. */
void RtspServer::MediaPreparedCb(GstRTSPMedia * media)
{
  guint i, n_streams;

  n_streams = gst_rtsp_media_n_streams(media);

  GST_INFO("media %p is prepared and has %u streams", media, n_streams);

  for(i = 0; i < n_streams; i++) {
    GstRTSPStream *stream;
    GObject *session;

    stream = gst_rtsp_media_get_stream(media, i);
    if(stream == NULL)
      continue;

    session = gst_rtsp_stream_get_rtpsession(stream);
    GST_INFO("watching session %p on stream %u", session, i);

    g_signal_connect(session, "on-ssrc-active",
      (GCallback) OnSsrcActive, media);
    g_signal_connect(session, "on-sender-ssrc-active",
      (GCallback) OnSenderSsrcActive, media);
  }
}

void RtspServer::MediaConfigureCb(GstRTSPMediaFactory * factory, GstRTSPMedia * media)
{
  /* connect our prepared signal so that we can see when this media is
   * prepared for streaming */
  g_signal_connect(media, "prepared",(GCallback) MediaPreparedCb, factory);
}

RtspServer::RtspServer(string pipeline) {
    m_pipeline = g_strdup_printf(pipeline.c_str());
}

void RtspServer::RtspServerInit(bool setcallback, const gchar *mountpoint) {
  gst_init(NULL, NULL);

  loop = g_main_loop_new(NULL, FALSE);

  server = gst_rtsp_server_new();
  g_object_set(server, "service", port, NULL);

  mounts = gst_rtsp_server_get_mount_points(server);

  factory = gst_rtsp_media_factory_new();
  gst_rtsp_media_factory_set_launch(factory, m_pipeline);
  if(setcallback) {
    g_signal_connect(factory, "media-configure", (GCallback)MediaConfigureCb, factory);
  }

  gst_rtsp_mount_points_add_factory(mounts, mountpoint, factory);
  g_object_unref(mounts);
}

void RtspServer::RtspServerAddUser(string user, string password, bool access_perm, bool construct_perm) {
  GstRTSPAuth *auth;
  GstRTSPToken *token;
  gchar *basic;
  gchar *user_char = const_cast<char*>(user.c_str());
  gchar *password_char = const_cast<char*>(password.c_str());

  auth = gst_rtsp_auth_new();
  gst_rtsp_media_factory_add_role(factory, user_char,
                                  GST_RTSP_PERM_MEDIA_FACTORY_ACCESS, G_TYPE_BOOLEAN, access_perm,
                                  GST_RTSP_PERM_MEDIA_FACTORY_CONSTRUCT, G_TYPE_BOOLEAN, construct_perm, NULL);
  
  token = gst_rtsp_token_new(GST_RTSP_TOKEN_MEDIA_FACTORY_ROLE, G_TYPE_STRING, user_char, NULL);
  basic = gst_rtsp_auth_make_basic(user_char, password_char);
  gst_rtsp_auth_add_basic(auth, basic, token);
  g_free(basic);
  gst_rtsp_token_unref(token);

  gst_rtsp_server_set_auth(server, auth);
  g_object_unref(auth);
}

int RtspServer::RtspStart() {
  if(gst_rtsp_server_attach(server, NULL) == 0) {
    g_print("Unable to attach Server\n");
    return -1;
  }
    
  g_unix_signal_add(SIGINT, SignalHandler, loop);

  g_main_loop_run(loop);
}

void RtspServer::RtspStop() {
  cout << "RtspServer: Stopping" << endl;
  PoolCleanup(server);
  RemoveSessions(server);
  g_object_unref(mounts);
  //mounts = gst_rtsp_server_get_mount_points(server);
  //gst_rtsp_mount_points_remove_factory(mounts, "/test");
  //g_object_unref(mounts);
  g_main_loop_quit(loop);
  //g_main_loop_unref(loop);
}

RtspServer::~RtspServer() {
  RtspStop();
}

int main(int argc, char *argv[]) {
  string pipeline = " ( v4l2src ! videoscale ! "
                        "capsfilter caps=video/x-raw ! videoconvert ! clockoverlay ! "
                        "queue ! x264enc ! rtph264pay name=pay0 pt=96 ) ";
  RtspServer rtspServer(pipeline);
  rtspServer.RtspServerInit(false, "/test");
  rtspServer.RtspServerAddUser("admin", "password", true, true);
  rtspServer.RtspStart();
}
