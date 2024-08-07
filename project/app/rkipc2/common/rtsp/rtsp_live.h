
#ifdef __cplusplus
// Link with C way
extern "C" {
#endif

int rtsp_live_init();

int rtsp_create_chaneal_session(char *dir);

int rtsp_start();

int rtsp_put(char *buf, int len, int type);

int rtsp_stop();

int rtsp_begin();

#ifdef __cplusplus
}
#endif