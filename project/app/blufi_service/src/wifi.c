/* SPDX-License-Identifier: MIT */
#include "wifi.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/evp.h>
#include <openssl/crypto.h>

struct control {int fd;char path[108];};
static int request(struct control *c,const char *cmd,char *out,size_t size)
{
    if(send(c->fd,cmd,strlen(cmd),0)<0)return -1;
    struct pollfd p={c->fd,POLLIN,0};
    gint64 deadline=g_get_monotonic_time()+1500000;
    while(g_get_monotonic_time()<deadline){
        int left=(deadline-g_get_monotonic_time())/1000;
        if(poll(&p,1,left)!=1)return -1;
        ssize_t n=recv(c->fd,out,size-1,0);if(n<0)return -1;out[n]=0;
        if(out[0]=='<')continue;
        return n;
    }
    return -1;
}
static int okay(struct control *c,const char *cmd)
{char out[128];return request(c,cmd,out,sizeof(out))>=0 && !strcmp(out,"OK\n");}
static char *field(const char *s,const char *key)
{
    char *prefix=g_strdup_printf("%s=",key),*value=NULL;size_t n=strlen(prefix);
    for(const char *p=s;*p;){const char *end=strchr(p,'\n');if(!end)end=p+strlen(p);
        if((size_t)(end-p)>=n&&!strncmp(p,prefix,n)){value=g_strndup(p+n,end-p-n);break;}p=*end?end+1:end;}
    g_free(prefix);return value;
}
static int current_id(struct control *c, char *status,size_t n)
{if(request(c,"STATUS",status,n)<0)return -1;char *v=field(status,"id");int id=v?atoi(v):-1;g_free(v);return id;}
static void report(struct wifi_job *j,const char *status,int state)
{
    j->response[0]=1;j->response[1]=state;j->response[2]=0;j->response_len=3;
    /* SSID in STATUS is escaped; omit optional TLVs rather than misencode it. */
    (void)status;
}
static gboolean cancelled(GCancellable *c){return c && g_cancellable_is_cancelled(c);}
static int stop_owned_dhcp(void)
{
    char *contents=NULL;gsize length=0;
    if(!g_file_get_contents("/var/run/blufi-udhcpc.pid",&contents,&length,NULL))return 0;
    char *end=NULL;long pid=strtol(contents,&end,10);g_free(contents);
    if(pid<=1)return -1;
    char path[64];g_snprintf(path,sizeof(path),"/proc/%ld/cmdline",pid);
    if(!g_file_get_contents(path,&contents,&length,NULL)){unlink("/var/run/blufi-udhcpc.pid");return 0;}
    gboolean program=length && strstr(contents,"udhcpc"),interface=FALSE;
    for(gsize i=0;i+1<length;){const char *arg=contents+i;gsize n=strnlen(arg,length-i);i+=n+1;
        if(!strcmp(arg,"-i")&&i<length&&!strcmp(contents+i,"wlan0"))interface=TRUE;}
    g_free(contents);if(!program||!interface)return -1;
    if(kill(pid,SIGTERM)&&errno!=ESRCH)return -1;
    for(int i=0;i<20;i++){if(kill(pid,0)&&errno==ESRCH){unlink("/var/run/blufi-udhcpc.pid");return 0;}g_usleep(100000);}
    return -1;
}
static int dhcp(GCancellable *cancel)
{
    /* Sole wlan0 DHCP owner while provisioning. eth0 DHCP is untouched. */
    gchar *args[]={"udhcpc","-n","-t","5","-T","3","-i","wlan0","-p","/var/run/blufi-udhcpc.pid",NULL};
    GError *e=NULL;GSubprocess *p=g_subprocess_newv((const gchar *const *)args,G_SUBPROCESS_FLAGS_STDOUT_SILENCE|G_SUBPROCESS_FLAGS_STDERR_SILENCE,&e);
    if(!p){g_clear_error(&e);return -1;}
    gboolean ok=g_subprocess_wait_check(p,cancel,&e);if(cancelled(cancel)){g_subprocess_force_exit(p);g_subprocess_wait(p,NULL,NULL);}
    g_clear_error(&e);g_object_unref(p);return ok?0:-1;
}
static int persist(struct control *c, const char *path)
{
    gchar *old=NULL;gsize len=0;GError *error=NULL;
    if(!g_file_get_contents(path,&old,&len,&error)){g_clear_error(&error);return -1;}
    char *backup=g_strconcat(path,".blufi-last-good",NULL),*temp=g_strconcat(path,".tmp",NULL);
    int result=-1;
    if(!g_file_set_contents_full(backup,old,len,G_FILE_SET_CONTENTS_CONSISTENT|G_FILE_SET_CONTENTS_DURABLE,0600,&error))goto done;
    if(chmod(backup,0600))goto done;
    /* Supplicant fopen("w") preserves this private mode before its atomic rename. */
    int fd=open(temp,O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);
    if(fd<0)goto done;
    close(fd);
    gboolean saved=okay(c,"SAVE_CONFIG");unlink(temp);
    if(saved){
        fd=open(path,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
        if(fd>=0){saved=!fchmod(fd,0600)&&!fsync(fd);close(fd);}else saved=FALSE;
        char *dir=g_path_get_dirname(path);fd=open(dir,O_RDONLY|O_DIRECTORY|O_CLOEXEC);g_free(dir);
        if(fd>=0){if(fsync(fd))saved=FALSE;close(fd);}else saved=FALSE;
    }
    if(saved)result=0;
    else {
        g_clear_error(&error);
        if(!g_file_set_contents_full(path,old,len,G_FILE_SET_CONTENTS_CONSISTENT|G_FILE_SET_CONTENTS_DURABLE,0600,&error))g_warning("Could not restore config; last-good backup retained");
    }
 done:
    g_clear_error(&error);OPENSSL_cleanse(old,len);g_free(old);g_free(backup);g_free(temp);return result;
}
static size_t unescape(const char *s,uint8_t *out,size_t max)
{
    size_t n=0;for(size_t i=0;s[i]&&n<max;i++){
        if(s[i]=='\\'&&s[i+1]){i++;if(s[i]=='x'&&s[i+1]&&s[i+2]&&g_ascii_isxdigit(s[i+1])&&g_ascii_isxdigit(s[i+2])){
            out[n++]=(g_ascii_xdigit_value(s[i+1])<<4)|g_ascii_xdigit_value(s[i+2]);i+=2;
        }else {out[n++]=s[i]=='n'?'\n':s[i]=='t'?'\t':s[i]=='r'?'\r':s[i];}}
        else out[n++]=s[i];
    }return n;
}
void wifi_work(GTask *task,gpointer source,gpointer data,GCancellable *cancel)
{
    (void)source;struct wifi_job *j=data;struct control c={.fd=-1};int new_id=-1,old_id=-1;char reply[16384],cmd[256],status[4096]={0};
    j->result=-1;g_strlcpy(j->detail,"supplicant unavailable",sizeof(j->detail));
    struct sockaddr_un local={.sun_family=AF_UNIX},remote={.sun_family=AF_UNIX};
    g_snprintf(c.path,sizeof(c.path),"/tmp/blufi-ctrl-%ld-%p",(long)getpid(),(void *)j);
    g_strlcpy(local.sun_path,c.path,sizeof(local.sun_path));g_strlcpy(remote.sun_path,j->socket_path,sizeof(remote.sun_path));
    c.fd=socket(AF_UNIX,SOCK_DGRAM|SOCK_CLOEXEC,0);if(c.fd<0)goto done;
    if(bind(c.fd,(void *)&local,sizeof(local))||connect(c.fd,(void *)&remote,sizeof(remote)))goto done;
    old_id=current_id(&c,status,sizeof(status));
    if(j->operation==0){char *state=field(status,"wpa_state"),*ip=field(status,"ip_address");report(j,status,state&&!strcmp(state,"COMPLETED")&&ip?0:1);g_free(state);g_free(ip);j->result=0;goto done;}
    if(j->operation==1){
        /* Subscribe before triggering scan, avoiding the completion-event race. */
        if(!okay(&c,"ATTACH"))goto done;
        if(!okay(&c,"SCAN")){g_strlcpy(j->detail,"scan busy",sizeof(j->detail));goto done;}
        gboolean scanned=FALSE;
        for(int i=0;i<150&&!cancelled(cancel);i++){struct pollfd p={c.fd,POLLIN,0};if(poll(&p,1,100)==1){ssize_t n=recv(c.fd,reply,sizeof(reply)-1,0);if(n>0){reply[n]=0;if(strstr(reply,"CTRL-EVENT-SCAN-RESULTS")){scanned=TRUE;break;}}}}
        if(!okay(&c,"DETACH")||!scanned){g_strlcpy(j->detail,"scan timeout",sizeof(j->detail));goto done;}
        if(request(&c,"SCAN_RESULTS",reply,sizeof(reply))<0)goto done;
        char **lines=g_strsplit(reply,"\n",-1);j->response_len=0;
        for(int i=1;lines[i];i++){char **cols=g_strsplit(lines[i],"\t",5);if(g_strv_length(cols)==5){uint8_t ssid[32];size_t n=unescape(cols[4],ssid,32);
            if(n&&j->response_len+n+2<=sizeof(j->response)){size_t k=j->response_len;j->response[k++]=n+1;j->response[k++]=(uint8_t)atoi(cols[2]);memcpy(j->response+k,ssid,n);j->response_len=k+n;}}
            g_strfreev(cols);}
        g_strfreev(lines);j->result=0;goto done;
    }
    /* Only WPA2 personal. Derive a hex PSK so no password enters commands or argv. */
    if(!j->ssid_len||j->ssid_len>32||j->password_len<8||j->password_len>63){g_strlcpy(j->detail,"invalid WPA2 credentials",sizeof(j->detail));goto done;}
    uint8_t key[32];char hexkey[65],hexssid[65];
    if(!PKCS5_PBKDF2_HMAC_SHA1((char *)j->password,j->password_len,j->ssid,j->ssid_len,4096,32,key))goto done;
    for(int i=0;i<32;i++)sprintf(hexkey+i*2,"%02x",key[i]);
    for(size_t i=0;i<j->ssid_len;i++)sprintf(hexssid+i*2,"%02x",j->ssid[i]);
    OPENSSL_cleanse(key,sizeof(key));OPENSSL_cleanse(j->password,sizeof(j->password));
    if(request(&c,"ADD_NETWORK",reply,sizeof(reply))<0||!g_ascii_isdigit(reply[0]))goto done;
    new_id=atoi(reply);g_snprintf(cmd,sizeof(cmd),"SET_NETWORK %d ssid %s",new_id,hexssid);if(!okay(&c,cmd))goto rollback;
    g_snprintf(cmd,sizeof(cmd),"SET_NETWORK %d psk %s",new_id,hexkey);int valid=okay(&c,cmd);OPENSSL_cleanse(hexkey,sizeof(hexkey));OPENSSL_cleanse(cmd,sizeof(cmd));if(!valid)goto rollback;
    g_snprintf(cmd,sizeof(cmd),"SET_NETWORK %d key_mgmt WPA-PSK",new_id);if(!okay(&c,cmd))goto rollback;
    g_snprintf(cmd,sizeof(cmd),"SET_NETWORK %d proto RSN",new_id);if(!okay(&c,cmd))goto rollback;
    g_snprintf(cmd,sizeof(cmd),"SET_NETWORK %d scan_ssid 1",new_id);if(!okay(&c,cmd))goto rollback;
    if(stop_owned_dhcp()){g_strlcpy(j->detail,"DHCP owner did not stop",sizeof(j->detail));goto rollback;}
    g_snprintf(cmd,sizeof(cmd),"SELECT_NETWORK %d",new_id);if(!okay(&c,cmd))goto rollback;
    gboolean associated=FALSE;
    for(int i=0;i<60&&!cancelled(cancel);i++){g_usleep(500000);int id=current_id(&c,status,sizeof(status));char *state=field(status,"wpa_state");associated=id==new_id&&state&&!strcmp(state,"COMPLETED");g_free(state);if(associated)break;}
    if(!associated){g_strlcpy(j->detail,"association timeout",sizeof(j->detail));goto rollback;}
    g_atomic_int_set(&j->phase,3);
    if(dhcp(cancel)){g_strlcpy(j->detail,"DHCP failed",sizeof(j->detail));goto rollback;}
    if(current_id(&c,status,sizeof(status))!=new_id)goto rollback;
    char *ip=field(status,"ip_address");if(!ip||!strcmp(ip,"0.0.0.0")){g_free(ip);goto rollback;}g_free(ip);
    if(persist(&c,j->config_path)){g_strlcpy(j->detail,"persistent config commit failed",sizeof(j->detail));goto rollback;}
    report(j,status,0);j->result=0;g_strlcpy(j->detail,"IP acquired, configuration saved",sizeof(j->detail));goto done;
 rollback:
    if(new_id>=0){g_snprintf(cmd,sizeof(cmd),"REMOVE_NETWORK %d",new_id);okay(&c,cmd);}
    if(old_id>=0){
        stop_owned_dhcp();g_snprintf(cmd,sizeof(cmd),"SELECT_NETWORK %d",old_id);okay(&c,cmd);
        for(int i=0;i<40&&!cancelled(cancel);i++){g_usleep(500000);int id=current_id(&c,status,sizeof(status));char *state=field(status,"wpa_state");gboolean restored=id==old_id&&state&&!strcmp(state,"COMPLETED");g_free(state);if(restored){dhcp(cancel);break;}}
    }
    report(j,"",1);
 done:
    if(c.fd>=0)close(c.fd);unlink(c.path);OPENSSL_cleanse(j->password,sizeof(j->password));g_task_return_boolean(task,j->result==0);
}
