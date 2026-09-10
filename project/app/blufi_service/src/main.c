/* SPDX-License-Identifier: MIT */
#include "protocol.h"
#include "wifi.h"
#include <glib-unix.h>
#include <linux/input.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <openssl/crypto.h>
#define ROOT "/com/luckfox/blufi"
#define SVC ROOT "/service0"
#define RX SVC "/char0"
#define TX SVC "/char1"
#define ADV ROOT "/advertisement0"
#define ADAPTER "/org/bluez/hci0"
#define UUID_SERVICE "0000ffff-0000-1000-8000-00805f9b34fb"
#define UUID_RX "0000ff01-0000-1000-8000-00805f9b34fb"
#define UUID_TX "0000ff02-0000-1000-8000-00805f9b34fb"
struct app {
    GMainLoop *loop;GDBusConnection *bus;struct bf protocol;GQueue tx;
    gboolean notify,ready,advertising,registering,window,busy,quitting,held,armed;
    gint64 deadline,pressed,mode_since,last_rx;int led_mode,keyfd,lockfd;unsigned generation;
    char *peer,*led,*socket,*config,*input,*name,*trigger,*brightness;
    uint8_t ssid[32],pass[64];size_t ssid_len,pass_len;
    GCancellable *cancel; struct wifi_job *active_job;
};
static const char xml[]=
"<node>"
"<interface name='org.freedesktop.DBus.ObjectManager'><method name='GetManagedObjects'><arg type='a{oa{sa{sv}}}' direction='out'/></method></interface>"
"<interface name='org.bluez.GattService1'><property name='UUID' type='s' access='read'/><property name='Primary' type='b' access='read'/></interface>"
"<interface name='org.bluez.GattCharacteristic1'>"
"<method name='WriteValue'><arg type='ay' direction='in'/><arg type='a{sv}' direction='in'/></method>"
"<method name='StartNotify'/><method name='StopNotify'/>"
"<property name='UUID' type='s' access='read'/><property name='Service' type='o' access='read'/><property name='Flags' type='as' access='read'/><property name='Value' type='ay' access='read'/><property name='Notifying' type='b' access='read'/></interface>"
"<interface name='org.bluez.LEAdvertisement1'><method name='Release'/><property name='Type' type='s' access='read'/><property name='ServiceUUIDs' type='as' access='read'/><property name='LocalName' type='s' access='read'/><property name='Includes' type='as' access='read'/></interface>"
"</node>";
static void log_state(const char *s){g_message("%s",s);}
static void write_led(struct app *a,const char *file,const char *value)
{char *p=g_build_filename(a->led,file,NULL);int fd=open(p,O_WRONLY|O_CLOEXEC);if(fd>=0){if(write(fd,value,strlen(value))<0)g_warning("LED write failed");close(fd);}g_free(p);}
static void restore_led(struct app *a){write_led(a,"trigger",a->trigger?a->trigger:"none");if(!a->trigger||!strcmp(a->trigger,"none"))write_led(a,"brightness",a->brightness?a->brightness:"0");a->led_mode=0;}
static void led(struct app *a,int mode){a->led_mode=mode;a->mode_since=g_get_monotonic_time();if(mode)write_led(a,"trigger","none");else restore_led(a);}
static void disconnect_peer(struct app *a,const char *peer)
{if(peer)g_dbus_connection_call(a->bus,"org.bluez",peer,"org.bluez.Device1","Disconnect",NULL,NULL,G_DBUS_CALL_FLAGS_NONE,3000,NULL,NULL,NULL);}
static void reset_session(struct app *a)
{bf_clear(&a->protocol);g_queue_clear_full(&a->tx,(GDestroyNotify)g_bytes_unref);a->notify=FALSE;g_clear_pointer(&a->peer,g_free);OPENSSL_cleanse(a->pass,sizeof(a->pass));a->pass_len=0;a->ssid_len=0;a->generation++;}
static void stop_advertising(struct app *a)
{if(a->advertising){g_dbus_connection_call(a->bus,"org.bluez",ADAPTER,"org.bluez.LEAdvertisingManager1","UnregisterAdvertisement",g_variant_new("(o)",ADV),NULL,0,5000,NULL,NULL,NULL);a->advertising=FALSE;}}
static void close_window(struct app *a)
{a->window=FALSE;stop_advertising(a);disconnect_peer(a,a->peer);if(!a->busy)led(a,0);log_state("pairing window closed");}
static void adv_done(GObject *source,GAsyncResult *r,gpointer data)
{struct app *a=data;GError *e=NULL;GVariant *v=g_dbus_connection_call_finish(G_DBUS_CONNECTION(source),r,&e);a->registering=FALSE;
 if(!v){g_warning("advertisement: %s",e->message);g_clear_error(&e);a->window=FALSE;led(a,0);return;}g_variant_unref(v);a->advertising=TRUE;if(!a->window)stop_advertising(a);else log_state("BluFi advertising ready");}
static void open_window(struct app *a)
{if(!a->ready||a->window||a->busy||a->registering)return;reset_session(a);a->window=TRUE;a->deadline=g_get_monotonic_time()+180*G_USEC_PER_SEC;led(a,1);a->registering=TRUE;
 g_dbus_connection_call(a->bus,"org.bluez",ADAPTER,"org.bluez.LEAdvertisingManager1","RegisterAdvertisement",g_variant_new("(oa{sv})",ADV,NULL),NULL,0,10000,NULL,adv_done,a);}
static gboolean pair_signal(gpointer data){open_window(data);return G_SOURCE_CONTINUE;}
static void emit(void *data,const uint8_t *p,size_t n)
{struct app *a=data;if(g_queue_get_length(&a->tx)>=512){disconnect_peer(a,a->peer);return;}g_queue_push_tail(&a->tx,g_bytes_new(p,n));}
static void error_reply(struct app *a,uint8_t error){bf_send(&a->protocol,0x49,&error,1);}
struct task_info {struct app *a;struct wifi_job job;unsigned generation;};
static void job_done(GObject *source,GAsyncResult *r,gpointer data)
{(void)source;struct task_info *t=data;struct app *a=t->a;GError *e=NULL;g_task_propagate_boolean(G_TASK(r),&e);g_clear_error(&e);
 a->busy=FALSE;a->active_job=NULL;g_clear_object(&a->cancel);g_message("Wi-Fi operation %d: %s",t->job.operation,t->job.result==0?"complete":t->job.detail);
 if(!a->quitting && t->generation==a->generation && a->peer){if(!t->job.result)bf_send(&a->protocol,t->job.operation==1?0x45:0x3d,t->job.response,t->job.response_len);else {if(t->job.operation!=2)error_reply(a,t->job.operation==1?11:12);if(t->job.operation==2)bf_send(&a->protocol,0x3d,(uint8_t[]){1,1,0},3);}}
 if(t->job.operation==2)led(a,t->job.result==0?3:4);
 OPENSSL_cleanse(t->job.password,sizeof(t->job.password));g_free(t);
 if(a->quitting)g_main_loop_quit(a->loop);
}
static void start_job(struct app *a,int op)
{if(a->busy){error_reply(a,9);return;}struct task_info *t=g_new0(struct task_info,1);t->a=a;t->generation=a->generation;t->job.operation=op;t->job.socket_path=a->socket;t->job.config_path=a->config;
 memcpy(t->job.ssid,a->ssid,a->ssid_len);t->job.ssid_len=a->ssid_len;memcpy(t->job.password,a->pass,a->pass_len);t->job.password_len=a->pass_len;
 if(op==2){OPENSSL_cleanse(a->pass,sizeof(a->pass));a->pass_len=0;led(a,2);}a->busy=TRUE;a->active_job=&t->job;g_atomic_int_set(&t->job.phase,2);a->cancel=g_cancellable_new();
 GTask *task=g_task_new(NULL,a->cancel,job_done,t);g_task_set_task_data(task,&t->job,NULL);g_task_set_return_on_cancel(task,FALSE);g_task_run_in_thread(task,wifi_work);g_object_unref(task);}
static void message(void *data,uint8_t type,const uint8_t *v,size_t n)
{struct app *a=data;g_message("BluFi message type=0x%02x length=%zu",type,n);
 switch(type){
 case 0:break;
 case 1:if(bf_negotiate(&a->protocol,v,n)){error_reply(a,6);disconnect_peer(a,a->peer);}break;
 case 4:if(n!=1||!a->protocol.keyed||v[0]!=3){error_reply(a,9);break;}a->protocol.mode=v[0];log_state("BluFi encrypted session established");break;
 case 8:if(n!=1||v[0]!=1)error_reply(a,9);break;
 case 9:if(!n||n>32||a->busy){error_reply(a,9);break;}memcpy(a->ssid,v,n);a->ssid_len=n;OPENSSL_cleanse(a->pass,sizeof(a->pass));a->pass_len=0;break;
 case 13:if(n<8||n>63||a->busy){error_reply(a,9);break;}memcpy(a->pass,v,n);a->pass_len=n;break;
 case 12:if(n||!a->protocol.keyed||!a->ssid_len||a->pass_len<8){error_reply(a,9);break;}start_job(a,2);break;
 case 20:if(n){error_reply(a,9);break;}if(a->active_job&&a->active_job->operation==2){uint8_t state[]={1,g_atomic_int_get(&a->active_job->phase),0};bf_send(&a->protocol,0x3d,state,3);}else start_job(a,0);break;
 case 28:if(!n)bf_send(&a->protocol,0x41,(uint8_t[]){1,3},2);break;
 case 32:disconnect_peer(a,a->peer);break;
 case 36:if(!n)start_job(a,1);else error_reply(a,9);break;
 case 5:if(n!=6)error_reply(a,9);break; /* Optional BSSID; SSID selection remains automatic. */
 default:error_reply(a,9);break;
 }}
static GVariant *property(GDBusConnection *c,const gchar *sender,const gchar *path,const gchar *iface,const gchar *name,GError **err,gpointer data)
{(void)c;(void)sender;(void)iface;(void)err;struct app *a=data;gboolean isrx=!strcmp(path,RX);
 if(!strcmp(name,"UUID"))return g_variant_new_string(!strcmp(path,SVC)?UUID_SERVICE:isrx?UUID_RX:UUID_TX);
 if(!strcmp(name,"Primary"))return g_variant_new_boolean(TRUE);
 if(!strcmp(name,"Service"))return g_variant_new_object_path(SVC);
 if(!strcmp(name,"Flags"))return g_variant_new_strv(isrx?(const char*[]){"write","write-without-response",NULL}:(const char*[]){"notify",NULL},-1);
 if(!strcmp(name,"Value"))return g_variant_new_fixed_array(G_VARIANT_TYPE_BYTE,NULL,0,1);
 if(!strcmp(name,"Notifying"))return g_variant_new_boolean(a->notify);
 if(!strcmp(name,"Type"))return g_variant_new_string("peripheral");
 if(!strcmp(name,"ServiceUUIDs"))return g_variant_new_strv((const char*[]){UUID_SERVICE,NULL},-1);
 if(!strcmp(name,"LocalName"))return g_variant_new_string(a->name);
 if(!strcmp(name,"Includes"))return g_variant_new_strv((const char*[]){"tx-power",NULL},-1);
 return NULL;}
static void method(GDBusConnection *c,const gchar *sender,const gchar *path,const gchar *iface,const gchar *name,GVariant *params,GDBusMethodInvocation *call,gpointer data)
{struct app *a=data;(void)c;(void)sender;(void)iface;
 if(!strcmp(name,"GetManagedObjects")){
  GVariantBuilder objects;g_variant_builder_init(&objects,G_VARIANT_TYPE("a{oa{sa{sv}}}"));
  const char *paths[]={SVC,RX,TX};const char *sn[]={"UUID","Primary",NULL},*cn[]={"UUID","Service","Flags","Value","Notifying",NULL};
  for(int i=0;i<3;i++){GVariantBuilder interfaces,props;g_variant_builder_init(&interfaces,G_VARIANT_TYPE("a{sa{sv}}"));g_variant_builder_init(&props,G_VARIANT_TYPE("a{sv}"));
   const char **names=i?cn:sn;for(int j=0;names[j];j++)g_variant_builder_add(&props,"{sv}",names[j],property(NULL,NULL,paths[i],NULL,names[j],NULL,a));
   g_variant_builder_add(&interfaces,"{sa{sv}}",i?"org.bluez.GattCharacteristic1":"org.bluez.GattService1",&props);g_variant_builder_add(&objects,"{oa{sa{sv}}}",paths[i],&interfaces);}
  g_dbus_method_invocation_return_value(call,g_variant_new("(a{oa{sa{sv}}})",&objects));return;
 }
 if(!strcmp(name,"WriteValue")&&!strcmp(path,RX)){
  GVariant *bytes,*opts;g_variant_get(params,"(@ay@a{sv})",&bytes,&opts);gsize n=0;const uint8_t *p=g_variant_get_fixed_array(bytes,&n,1);const char *dev=NULL;guint16 offset=0;
  g_variant_lookup(opts,"device","&o",&dev);g_variant_lookup(opts,"offset","q",&offset);
  gboolean valid=a->window&&dev&&!offset&&(!a->peer||!strcmp(dev,a->peer));
  if(valid){if(!a->peer)a->peer=g_strdup(dev);a->last_rx=g_get_monotonic_time();valid=bf_receive(&a->protocol,p,n)==0;}
  gboolean own_peer=dev&&a->peer&&!strcmp(dev,a->peer);
  g_variant_unref(bytes);g_variant_unref(opts);
  if(!valid){g_dbus_method_invocation_return_dbus_error(call,"org.bluez.Error.NotPermitted","Invalid or inactive BluFi session");if(own_peer)disconnect_peer(a,a->peer);return;}
 }else if(!strcmp(name,"StartNotify")&&!strcmp(path,TX)){a->notify=TRUE;}
 else if(!strcmp(name,"StopNotify")&&!strcmp(path,TX)){a->notify=FALSE;}
 else if(!strcmp(name,"Release")){a->advertising=FALSE;}
 else {g_dbus_method_invocation_return_dbus_error(call,"org.bluez.Error.NotSupported","Unsupported operation");return;}
 g_dbus_method_invocation_return_value(call,NULL);
}
static const GDBusInterfaceVTable vtable={.method_call=method,.get_property=property};
static void changed(GDBusConnection *c,const gchar *sender,const gchar *path,const gchar *interface,const gchar *signal,GVariant *params,gpointer data)
{(void)c;(void)sender;(void)interface;(void)signal;struct app *a=data;const char *iface;GVariant *props,*invalid;
 g_variant_get(params,"(&s@a{sv}@as)",&iface,&props,&invalid);gboolean connected;
 if(!strcmp(iface,"org.bluez.Device1")&&g_variant_lookup(props,"Connected","b",&connected)){
  if(connected){if(!a->window||(a->peer&&strcmp(path,a->peer)))disconnect_peer(a,path);else if(!a->peer){reset_session(a);a->peer=g_strdup(path);log_state("BLE peer connected");}}
  else if(a->peer&&!strcmp(path,a->peer)){reset_session(a);log_state("BLE peer disconnected");}}
 g_variant_unref(props);g_variant_unref(invalid);
}
static void registered(GObject *source,GAsyncResult *r,gpointer data)
{struct app *a=data;GError *e=NULL;GVariant *v=g_dbus_connection_call_finish(G_DBUS_CONNECTION(source),r,&e);if(!v){g_warning("GATT registration: %s",e->message);g_clear_error(&e);g_main_loop_quit(a->loop);return;}g_variant_unref(v);a->ready=TRUE;log_state("GATT ready; hold Recovery-key 3 seconds or SIGUSR1 to pair");if(a->deadline==-1)open_window(a);}
static gboolean tick(gpointer data)
{struct app *a=data;gint64 now=g_get_monotonic_time();
 if(a->keyfd>=0){struct input_event ev;while(read(a->keyfd,&ev,sizeof(ev))==sizeof(ev)){
  if(ev.type==EV_SYN&&ev.code==SYN_DROPPED){a->held=FALSE;a->armed=FALSE;}
  if(ev.type==EV_KEY&&ev.code==KEY_0){if(!ev.value){a->held=FALSE;a->armed=TRUE;}else if(ev.value==1&&a->armed){a->held=TRUE;a->pressed=now;}}}
  if(a->held&&now-a->pressed>=3*G_USEC_PER_SEC){a->held=FALSE;a->armed=FALSE;open_window(a);}}
 if(a->protocol.used&&now-a->last_rx>10*G_USEC_PER_SEC)disconnect_peer(a,a->peer);
 if(a->window&&now>=a->deadline&&!a->busy&&a->led_mode!=3)close_window(a);
 if(a->led_mode){gint64 ms=(now-a->mode_since)/1000;gboolean on=FALSE;
  if(a->led_mode==1)on=ms%1000<500;else if(a->led_mode==2)on=ms%200<100;else if(a->led_mode==3)on=TRUE;else on=ms%1000<100||(ms%1000>=200&&ms%1000<300);
  write_led(a,"brightness",on?"255":"0");
  if(ms>=3000&&a->led_mode==3)close_window(a);else if(ms>=3000&&a->led_mode==4)led(a,a->window?1:0);
 }
 return G_SOURCE_CONTINUE;}
static gboolean send_tick(gpointer data)
{struct app *a=data;if(a->notify&&!g_queue_is_empty(&a->tx)){GBytes *b=g_queue_pop_head(&a->tx);gsize n;const void *p=g_bytes_get_data(b,&n);
 GVariantBuilder props;g_variant_builder_init(&props,G_VARIANT_TYPE("a{sv}"));g_variant_builder_add(&props,"{sv}","Value",g_variant_new_fixed_array(G_VARIANT_TYPE_BYTE,p,n,1));
 g_dbus_connection_emit_signal(a->bus,NULL,TX,"org.freedesktop.DBus.Properties","PropertiesChanged",g_variant_new("(sa{sv}as)","org.bluez.GattCharacteristic1",&props,NULL),NULL);g_bytes_unref(b);}return G_SOURCE_CONTINUE;}
static gboolean quit(gpointer data)
{struct app *a=data;a->quitting=TRUE;close_window(a);if(a->cancel)g_cancellable_cancel(a->cancel);else g_main_loop_quit(a->loop);return G_SOURCE_CONTINUE;}
int main(int argc,char **argv)
{
 umask(0077);
 struct app a={.keyfd=-1,.lockfd=-1};gboolean pair=FALSE;
 GOptionEntry options[]={{"pair-now",0,0,G_OPTION_ARG_NONE,&pair,"Open pairing window immediately",NULL},{"input",0,0,G_OPTION_ARG_FILENAME,&a.input,"gpio-keys evdev path",NULL},{"led",0,0,G_OPTION_ARG_FILENAME,&a.led,"LED class path",NULL},{"socket",0,0,G_OPTION_ARG_FILENAME,&a.socket,"wpa control socket",NULL},{"config",0,0,G_OPTION_ARG_FILENAME,&a.config,"supplicant config path",NULL},{"name",0,0,G_OPTION_ARG_STRING,&a.name,"BLE name",NULL},{NULL}};
 GError *e=NULL;GOptionContext *options_ctx=g_option_context_new("- Linux BluFi 1.3 provisioning");g_option_context_add_main_entries(options_ctx,options,NULL);if(!g_option_context_parse(options_ctx,&argc,&argv,&e)){g_printerr("%s\n",e->message);return 1;}
 if(!a.led)a.led=g_strdup("/sys/class/leds/work");if(!a.socket)a.socket=g_strdup("/var/run/wpa_supplicant/wlan0");if(!a.config)a.config=g_strdup("/userdata/wpa_supplicant.conf");if(!a.name)a.name=g_strdup("BLUFI_Luckfox");
 a.lockfd=open("/var/run/blufi_service.lock",O_CREAT|O_RDWR|O_CLOEXEC,0600);if(a.lockfd<0||flock(a.lockfd,LOCK_EX|LOCK_NB)){g_printerr("service already running or lock unavailable\n");return 1;}
 if(a.input)a.keyfd=open(a.input,O_RDONLY|O_NONBLOCK|O_CLOEXEC);
 else for(int i=0;i<32;i++){char p[64],name[128]={0};g_snprintf(p,sizeof(p),"/dev/input/event%d",i);int fd=open(p,O_RDONLY|O_NONBLOCK|O_CLOEXEC);if(fd<0)continue;
  unsigned long bits[(KEY_MAX+8*sizeof(long))/(8*sizeof(long))]={0};ioctl(fd,EVIOCGBIT(EV_KEY,sizeof(bits)),bits);ioctl(fd,EVIOCGNAME(sizeof(name)),name);
  if(!strcmp(name,"gpio-keys")&&(bits[KEY_0/(8*sizeof(long))]&(1UL<<(KEY_0%(8*sizeof(long)))))){a.keyfd=fd;break;}close(fd);}
 if(a.keyfd>=0){unsigned long keys[(KEY_MAX+8*sizeof(long))/(8*sizeof(long))]={0};ioctl(a.keyfd,EVIOCGKEY(sizeof(keys)),keys);a.armed=!(keys[KEY_0/(8*sizeof(long))]&(1UL<<(KEY_0%(8*sizeof(long)))));}else g_warning("Recovery-key unavailable; SIGUSR1 can open pairing");
 char *p=g_build_filename(a.led,"trigger",NULL),*tr=NULL;g_file_get_contents(p,&tr,NULL,NULL);g_free(p);if(tr){char *s=strchr(tr,'['),*end=s?strchr(s,']'):NULL;if(end)a.trigger=g_strndup(s+1,end-s-1);g_free(tr);}p=g_build_filename(a.led,"brightness",NULL);g_file_get_contents(p,&a.brightness,NULL,NULL);g_free(p);
 a.loop=g_main_loop_new(NULL,FALSE);bf_init(&a.protocol,&a,emit,message);g_queue_init(&a.tx);a.bus=g_bus_get_sync(G_BUS_TYPE_SYSTEM,NULL,&e);if(!a.bus){g_printerr("D-Bus: %s\n",e->message);return 1;}
 GDBusNodeInfo *node=g_dbus_node_info_new_for_xml(xml,&e);const char *paths[]={ROOT,SVC,RX,TX,ADV};int indices[]={0,1,2,2,3};
 for(int i=0;i<5;i++)if(!g_dbus_connection_register_object(a.bus,paths[i],node->interfaces[indices[i]],&vtable,&a,NULL,&e)){g_printerr("register: %s\n",e->message);return 1;}
 g_dbus_connection_signal_subscribe(a.bus,"org.bluez","org.freedesktop.DBus.Properties","PropertiesChanged",NULL,NULL,0,changed,&a,NULL);
 GVariant *powered=NULL;
 for(int attempt=0;attempt<20&&!powered;attempt++){
  g_clear_error(&e);
  powered=g_dbus_connection_call_sync(a.bus,"org.bluez",ADAPTER,"org.freedesktop.DBus.Properties","Set",g_variant_new("(ssv)","org.bluez.Adapter1","Powered",g_variant_new_boolean(TRUE)),NULL,0,2000,NULL,&e);
  if(!powered)g_usleep(250000);
 }
 if(!powered){g_printerr("hci0 power: %s\n",e->message);return 1;}g_variant_unref(powered);
 a.deadline=pair?-1:0;g_dbus_connection_call(a.bus,"org.bluez",ADAPTER,"org.bluez.GattManager1","RegisterApplication",g_variant_new("(oa{sv})",ROOT,NULL),NULL,0,10000,NULL,registered,&a);
 g_unix_signal_add(SIGUSR1,pair_signal,&a);g_unix_signal_add(SIGTERM,quit,&a);g_unix_signal_add(SIGINT,quit,&a);g_timeout_add(100,tick,&a);g_timeout_add(20,send_tick,&a);g_main_loop_run(a.loop);
 restore_led(&a);bf_clear(&a.protocol);g_queue_clear_full(&a.tx,(GDestroyNotify)g_bytes_unref);if(a.keyfd>=0)close(a.keyfd);close(a.lockfd);return a.ready?0:1;
}
