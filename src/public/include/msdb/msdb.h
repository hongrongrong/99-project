#ifndef __MSDB_H__
#define __MSDB_H__
#include "msstd.h"
#include "msdefs.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MS_DB_VERSION 			"1.0"
#define DB_BACKUP_NAME			"msdb.db"
#define DB_BACKUP_PASSWD		"ms120610"

#define MAX_LEN_MAP 			320
#define MAX_NUM_LEVEL 			4
#define NAS_MOUNT_NUM 			5

//schedule table
#define MAX_DAY_NUM     		8
#define MAX_PLAN_NUM_PER_DAY    12
#define MAX_WND_NUM             4
#define MAX_HTTPS_STR           64
#define MAX_PUSH_TYPE			9

//db file path
#define SQLITE_INIT_SQL_PATH	"/opt/app/db/"
#define SQLITE_INIT_FILE_PATH 	"/opt/app/msdb.db"

#define SQLITE_INIT_MYDB_PATH	"/opt/app/mydb"

//#define SQLITE_INIT_OEM_PATH 	"/opt/app/oem.db"

#define SQLITE_FILE_NAME		"/mnt/nand/app/msdb.db"
#define SQLITE_OEM_NAME			"/mnt/nand2/oem.db"//oem mount block

// HTTPS SSL
#define SSL_DEFAULT_PATH		"/mnt/nand/ssl/tmp"
#define SSL_FILE_PATH 			"/mnt/nand/ssl"
#define SSL_KEY_NAME 			"ssl_key.pem" 
#define SSL_CERT_NAME 			"ssl_cert.pem"
#define SSL_REQUEST_NAME 		"ssl_cert.csr"
#define SSL_CONFIG				"/etc/openssl/ssl/openssl.cnf"
#define SSL_CA_CERT				"ssl_ca_crt.pem"
#define SSL_CA_KEY 				"ssl_ca_key.pem"
#define SSL_CERT_SAVE_NAME		"ssl_cert_save.pem"

//param key
#define PARAM_DB_VERSION			"dbversion"
#define PARAM_MSSOCK_PATH_KEY		"mssockpath"
#define PARAM_DEBUG_LEVEL			"debug"
#define PARAM_DEBUG_ENABLE			"mscli"
#define PARAM_NOR_DI_STATUS			"dinor"
#define PARAM_NOR_DO_STATUS			"donor"
#define PARAM_ANONY					"anony"
#define PARAM_THREAD_SWITCH_PRI		"swpri"
#define PARAM_THREAD_REC_PRI		"recpri"

#define PARAM_DEVICE_ID				"deviceid"
#define PARAM_DEVICE_NO				"prefix"
#define PARAM_DEVICE_SN				"sn_code"
#define PARAM_DEVICE_MODEL			"model"
#define PARAM_DEVICE_COMPANY		"company"
#define PARAM_DEVICE_SV				"software_version"
#define PARAM_DEVICE_HV				"hardware_version"
#define PARAM_DEVICE_DEF_LANG		"deflang"
#define PARAM_DEVICE_LANG			"lang"
#define PARAM_DEVICE_HOTPLUG		"hdmihotplug"
#define PARAM_PLUG_VER				"plugin_version"
#define PARAM_OEM_TYPE				"oem_type"
#define PARAM_MAX_DISK				"max_disk"
#define PARAM_MAX_CAM				"max_cameras"
#define PARAM_MAX_PB_CAM			"max_pb_cam"
#define PARAM_MAX_ALARM_IN			"alarm_in"
#define PARAM_MAX_ALARM_OUT			"alarm_out"
#define PARAM_MAX_AUDIO_IN			"audio_in"
#define PARAM_MAX_AUDIO_OUT			"audio_out"
#define PARAM_MAX_NETWORKS			"max_lan"
#define PARAM_MAX_SCREEN			"max_screen"
#define PARAM_MAX_HDMI				"max_hdmi"
#define PARAM_MAX_VGA				"max_vga"
#define PARAM_CAMERA_LAYOUT			"max_layout"
#define PARAM_MAX_RS485				"max_rs485"
#define PARAM_DECODE				"decode"
#define PARAM_PUSH_MSG				"enable_push_msg"
#define PARAM_FRAME_BUFFER			"frame_buff"
#define PARAM_RAID_MODE				"raidmode"
#define PARAM_POE_PSD				"poepsd"
#define PARAM_REPLACE				"msdb_replacement"
#define PARAM_POE_NUM				"max_poe_num"
#define PARAM_LAYOUT1_CHNID			"layout1_chnid"
#define PARAM_SUB_LAYOUT1_CHNID		"sub_layout1_chnid"
#define PARAM_MPLUG_VER				"mplugin_version"
#define PARAM_SDK_VERSION			"sdkversion"
#define PARAM_POWER_KEY		        "power_key"
#define PARAM_MS_DDNS		        "msddns"

#define PARAM_RECYCLE_MODE			"recycle_mode"
#define PARAM_ESATA_TYPE			"esata_type"
#define PARAM_DISK_MODE				"group_mode"
#define PARAM_MSFS_DBUP				"msfs_dbup"
#define PARAM_HTTPS_ENABLE			"httpsenable"
#define PARAM_ACCESS_ENABLE         "ch_access"
#define PARAM_PUSH_TYPE				"push_type"
#define PARAM_FAILOVER_MODE		    "failover_mode"


//GUI
#define PARAM_GUI_CONFIG			"layout_use_one_config"
#define PARAM_GUI_CHAN_COLOR 		"channel_name_color"
#define PARAM_GUI_SKIP_BLANK		"skip_blank_page"
#define PARAM_GUI_AUTH				"local_auth_enable"
#define PARAM_GUI_MENU_AUTH			"menu_auth_enable"
#define PARAM_GUI_MENU_TIME_OUT		"menu_timeout"
#define PARAM_GUI_WIZARD_ENABLE		"wizard_enable"
#define PARAM_GUI_DISPLAY_MODE 		"display_mode"

#define PARAM_PUSH_VIDEO_STREAM		"video_push_stream"

/* ******************************* STRUCTURE ******************************* */
/*                                                                           */
/*                                                                           */
/* ************************************************************************* */
struct osd{
    int id;
    char name[64];
    int show_name;
    int show_date;
    int show_week;
    int date_format;
    int time_format;
    double date_pos_x;
    double date_pos_y;
    double name_pos_x;
    double name_pos_y;
    int alpha;
};

struct motion{
    int id;
    int enable;
    int sensitivity;
    unsigned int tri_channels;
    unsigned int tri_alarms;
    unsigned char motion_table[MAX_MOTION_CELL];
    int buzzer_interval;
    int ipc_sched_enable;
	char tri_channels_ex[66];
	int email_enable;
	int email_buzzer_interval;
};
struct motion_batch{
	int chanId[64];
	int size;
};
struct schdule_item{
    char start_time[32];
    char end_time[32];
    int action_type;
};

struct schedule_day{
    struct schdule_item schedule_item[MAX_PLAN_NUM_PER_DAY*MAX_WND_NUM];
    int wholeday_enable;
    int wholeday_action_type;
};

struct motion_schedule{
    struct schedule_day schedule_day[MAX_DAY_NUM];
};

struct video_loss{
    int id;
    int enable;
    unsigned int tri_alarms;
    int buzzer_interval;
	int email_enable;
	int email_buzzer_interval; 
};

struct video_loss_schedule{
    struct schedule_day schedule_day[MAX_DAY_NUM];
};

struct alarm_out{
    int id;
    char name[MAX_LEN_256];
    int type;
    int enable;
    int duration_time;
//    int buzzer_interval;
};

struct alarm_out_schedule{
    struct schedule_day schedule_day[MAX_DAY_NUM];
};

struct camera{
    int id;
	int type;
    int enable;

    int covert_on;
    int brightness;
    int contrast;
    int saturation;
    char username[64];
    char password[64];

    char ip_addr[64];//hrz.milesight here modify for ipv6 32-->64

    int main_rtsp_port;
    char main_source_path[256];

    int sub_rtsp_enable;

    int sub_rtsp_port;
    char sub_source_path[256];

    int manage_port;
    int camera_protocol;
    int transmit_protocol;
    int play_stream;
    int record_stream;

    int sync_time;
    int codec;
    char main_rtspurl[MAX_LEN_256];
    char sub_rtspurl[MAX_LEN_256];

    int poe_channel;
    char mac_addr[32];
    int physical_port;
	//int connect_mode; 
};

struct record{
    int id;
    int mode;
    int input_video_signal_type;
    int codec_type;
    int resolution;
    int fps;
    int kbps;
    int iframe_interval;
    int bitrate_type;
    int audio_enable;
    int audio_codec_type;
    int audio_samplerate;
    int prev_record_on;
    int prev_record_duration;
    int post_record_on;
    int post_record_duration;
	int record_expiration_date;
	int photo_expiration_date;
};

struct record_schedule{
    struct schedule_day schedule_day[MAX_DAY_NUM];
    int enable;
};

struct holiday{
    int id;
    char name[MAX_LEN_256];
    int enable;
    int type;
    int start_year;
    int start_mon;
    int start_mday;
    int start_mweek;
    int start_wday;
    int end_year;
    int end_mon;
    int end_mday;
    int end_mweek;
    int end_wday;
};

struct ptz_port{
    int id;
    int baudrate;
    int data_bit;
    int stop_bit;
    int parity_type;
    int protocol;
    int address;
    int com_type;
    int connect_type;
};

struct alarm_in{
    int id;
    int enable;
    char name[MAX_LEN_256];
    int type;
    unsigned int tri_channels;
    unsigned int tri_alarms;
    int buzzer_interval;
	char tri_channels_ex[66];
	//modify for action PZT preset & patrol
	int acto_ptz_channel;
	int acto_ptz_preset_enable;
	int acto_ptz_preset;
	int acto_ptz_patrol_enable;
	int acto_ptz_patrol;
	//for email 
	int email_enable;
	int email_buzzer_interval; 
	char tri_channels_snapshot[66];//for snapshot
};

struct alarm_in_schedule{
    struct schedule_day schedule_day[MAX_DAY_NUM];
};

struct db_user{
    int id;
    int enable;
    char username[MAX_USER_LEN*2];
    char password[MAX_PWD_LEN+1];
    int type;
    int permission;
    int remote_permission;
	unsigned long local_live_view;
	unsigned long local_playback;
	unsigned long remote_live_view;
	unsigned long remote_playback;
	char local_live_view_ex[66];
	char local_playback_ex[66];
	char remote_live_view_ex[66];
	char remote_playback_ex[66];
	char password_ex[MAX_PWD_LEN+1];
};

struct db_user_oem{
    int id;
    int enable;
    char username[MAX_USER_LEN*2];
    char password[MAX_PWD_LEN+1];
    int type;
    int permission;
    int remote_permission;
	unsigned long local_live_view;
	unsigned long local_playback;
	unsigned long remote_live_view;
	unsigned long remote_playback;
};

struct display{
    int main_resolution;
    int main_seq_enable;
    int main_seq_interval;
    int hdmi_audio_on;
    int audio_output_on;
    int volume;
    int stereo;

	int sub_enable;
    int sub_resolution;
    int sub_seq_enable;
    int sub_seq_interval;
    int spot_resolution;
    int spot_output_channel;

    int border_line_on;
    int date_format;
    int show_channel_name;
	int camera_info;
	int start_screen;
	int page_info;
	int eventPop_screen;
	int eventPop_time;
};

struct spot{
    int id;
    int seq_enable;
    int seq_interval;
    int input_channels;
};

struct time{
    int ntp_enable;
    int dst_enable;
    char time_zone[32];
    char time_zone_name[32];
    char ntp_server[64];
};

struct pppoe{
    int enable;
    int auto_connect;
    char username[64];
    char password[64];
};

struct ddns{
    int enable;
    char domain[64];
    char username[32];
    char password[32];
    char host_name[128];
    char free_dns_hash[128];
    int update_freq;
	int http_port;
	int rtsp_port;
	char ddnsurl[128];
};

struct upnp{
	int enable;
	int type;
	char name[64];
	char extern_ip[64];
	int http_port;
	int http_status;
	int rtsp_port;
	int rtsp_status;
};


struct email_receiver{
    int id;
    char address[128];
    char name[64];
};

struct email{
    char username[64];
    char password[64];
    char smtp_server[64];
    int port;
    char sender_addr[128];
    char sender_name[64];
    int enable_tls;
    int enable_attach;
    int capture_interval;
    struct email_receiver receiver[EMAIL_RECEIVER_NUM];
};

struct network{
    int mode;
    char host_name[128];
    int miimon;
    int bond0_primary_net;
    int bond0_enable;
    int bond0_type;
    char bond0_ip_address[MAX_LEN_16];
    char bond0_netmask[MAX_LEN_16];
    char bond0_gateway[MAX_LEN_16];
    char bond0_primary_dns[MAX_LEN_16];
    char bond0_second_dns[MAX_LEN_16];
    int bond0_mtu;

    int lan1_enable;//0=disable(down),1=enable(up)
    int lan1_type;//0=manual,1=dhcp
    char lan1_ip_address[MAX_LEN_16];
    char lan1_netmask[MAX_LEN_16];
    char lan1_gateway[MAX_LEN_16];
    char lan1_primary_dns[MAX_LEN_16];
    char lan1_second_dns[MAX_LEN_16];
    int lan1_mtu;

    int lan2_enable;
    int lan2_type;
    char lan2_ip_address[MAX_LEN_16];
    char lan2_netmask[MAX_LEN_16];
    char lan2_gateway[MAX_LEN_16];
    char lan2_primary_dns[MAX_LEN_16];
    char lan2_second_dns[MAX_LEN_16];
    int lan2_mtu;
	//david.milesight
	char lan1_dhcp_gateway[MAX_LEN_16];
   	char lan2_dhcp_gateway[MAX_LEN_16];
    int tri_alarms;
	
	char lan1_ip6_address[MAX_LEN_64];//hrz.milesight
	char lan1_ip6_netmask[MAX_LEN_16];
    char lan1_ip6_gateway[MAX_LEN_64];
		
    char lan2_ip6_address[MAX_LEN_64];
	char lan2_ip6_netmask[MAX_LEN_16];
    char lan2_ip6_gateway[MAX_LEN_64];
    int lan1_ip6_dhcp;//0=manual,1=Router Advertisement,2=dhcp
    int lan2_ip6_dhcp;//0=manual,1=Router Advertisement,2=dhcp

    char bond0_ip6_address[MAX_LEN_64];
	char bond0_ip6_netmask[MAX_LEN_16];
    char bond0_ip6_gateway[MAX_LEN_64];//end
    int bond0_ip6_dhcp;
};

struct network_more{
    int enable_ssh;
    int ssh_port;
    int http_port;
    int rtsp_port;
    int sdk_port;
    char url[256];
    int url_enable;
	int https_port;
};

struct snmp
{
	int v1_enable;
	int v2c_enable;
	char write_community[MAX_LEN_64];
	char read_community[MAX_LEN_64];

	int v3_enable;
	char read_security_name[MAX_LEN_64];
	int read_level_security;//0=auth,priv 1=auth,no priv 2=no auth,no priv
	int read_auth_algorithm;//0=MD5 1=SHA
	char read_auth_password[MAX_LEN_64];
	int read_pri_algorithm;//0=DES 1=AES
	char read_pri_password[MAX_LEN_64];

	char write_security_name[MAX_LEN_64];
	int write_level_security;//0=auth,priv 1=auth,no priv 2=no auth,no priv
	int write_auth_algorithm;//0=MD5 1=SHA
	char write_auth_password[MAX_LEN_64];
	int write_pri_algorithm;//0=DES 1=AES
	char write_pri_password[MAX_LEN_64];

	int port;
};


struct audio_in{
    int id;
    int enable;
    int samplerate;
    int volume;
};

struct layout{
	int main_output;
	int sub_output;
	int main_layout_mode;
	int sub_layout_mode;
};

struct ptz_speed{
	int id;
	int pan;
	int tilt;
	int zoom;
	int focus;
	int timeout;
};


struct storage{
	int id;
	int recycle_mode;
};

struct ptz_key{
	int preset_id;
	int timeout;
	int speed;
};

struct ptz_tour{
    struct ptz_key keys[KEY_MAX];
};

struct ipc_protocol {
	int pro_id;
	char pro_name[24];
	int function;
	int enable;
	int display_model;
};

struct ipc_model {
	int mod_id;
	int pro_id;
	char mod_name[32];
	int alarm_type;
	int default_model;
};

struct mosaic{
    int layoutmode;
    int channels[MAX_CAMERA];
};

struct disk_maintain_info {
	int log;
	int photo;
};

struct p2p_info {
	int enable;
    char model[32];
    char company[128];
    char email[128];
    char dealer[128];
    char ipc[64];
};

struct mask_area{
    int area_id;
    int enable;
    int start_x;
    int start_y;
    int area_width;
    int area_height;
    int width;
    int height;
    int fill_color;
};

struct privacy_mask{
    int id;
    struct mask_area area[MAX_MASK_AREA_NUM];
};

/*sdk push alarm id*/
struct alarm_pid{
	char id[256];
	char sn[256];
	int enable;
	int from;
	int push_interval;
	long last_push;
};

struct trigger_alarms {
	int network_disconn;
	int disk_full;
	int record_fail;
	int disk_fail;
	int disk_unformat;
	int no_disk;
};

struct camera_io {
    int chanid;
    int enable;
    unsigned int tri_channels;
    unsigned int tri_actions;
	char tri_channels_ex[66];
};

struct volume_detect{
    int chanid;
    int enable;
    unsigned int tri_channels;
    unsigned int tri_actions;
	char tri_channels_ex[66];
};

struct ip_conflict{
	char dev[16];
	char ipaddr[16];
};
typedef enum DAY_INTERVAL{
	SUNDAY=0,
	MONDAY,
	TUESDAY,
	WEDNESDAY,
	THURSDAY,
	FRIDAY,
	SATURDAY,
	EVERYDAY,
}DAY_INTERVAL;
typedef struct{
	int enable;
	DAY_INTERVAL wday;
	int hour;
	int login;
	char username[100];
	int reboot;
}reboot_conf;


typedef struct diskInfo{
	int disk_port;						//disk port ID
	int enable;
	char disk_vendor[MAX_LEN_64];		//nas ip address
	char disk_address[MAX_LEN_64];		//nas ip address
	char disk_directory[MAX_LEN_128];	//nas directory
	int disk_type;						//local, eSata, Nas
	int disk_group;						//group ID
	int disk_property;					// R/W
	int raid_level;					// raid level
}DISK_INFO_S;

struct cert_info{
	int type;
	char country[3];
	char common_name[MAX_HTTPS_STR];
	int validity;
	char password[16];
	char province[MAX_HTTPS_STR];
	char region[MAX_HTTPS_STR];
	char organization[MAX_HTTPS_STR];
	char company[MAX_HTTPS_STR];
	char email[MAX_HTTPS_STR];
	char subject[256];
	char dates[128];
	char option_company[MAX_HTTPS_STR];
	char startTime[16];
	char endTime[16];
};

enum{
	PRIVATE_CERT=0,
	DIRECT_CERT,
	CERT_REQ,
};

struct other_poe_camera{
	int id;
	int other_poe_channel;
};

typedef struct groupInfo{
	int groupid;						//disk port ID
	char chnMaskl[MAX_LEN_64+1];
	char chnMaskh[MAX_LEN_64+1];
}GROUP_INFO_S;

struct smart_event_schedule{
    struct schedule_day schedule_day[MAX_DAY_NUM];
};

struct smart_event{
    int id;
    int enable;
	unsigned int tri_channels;
    char tri_channels_ex[66];
    unsigned int tri_alarms;
	int buzzer_interval;
	int email_interval;
};


//failover mode
enum {
    FAILOVER_MODE_DISABLE = 0,
    FAILOVER_MODE_MASTER,
    FAILOVER_MODE_SLAVE
};

//connect status
enum {
    CONNECT_ERROR           = -1,   //异常
    CONNECT_INIT            = 0,    //初始化状态 等待更新
    CONNECT_READY           = 1,    //Link is up (Ready)连接成功，Slave待命
    CONNECT_BUSY            = 2,    //Link is up (Busy)该热备机正在给其他Master热备或回传，不支持同时热备A，回传B
    CONNECT_RETURN          = 3,    //Link is up (Returning)该热备机正在给当前Master回传
    CONNECT_DISCONNECTED    = 4,    //Link is down (Network Disconnected) 该热备机已添加，但不在线（不在线鬼知道添加没添加？？）
    CONNECT_NOT_SUPPORT     = 5,    //该IP所在热备机不支持热备（如未启用热备，或版本不是热备版本）
    CONNECT_UNPAIRED        = 6,    //该热备机出未添加当前Master
    CONNECT_AUTH_FAILED     = 7,    //该IP所在热备机密码错误
    CONNECT_ERROR_OTHER     = 8,    //该IP所在热备机型号错误或带宽不足（如3798机型）    
};

struct failover_list{
    int id;
    int enable;
    char ipaddr[MAX_LEN_64];
    char mac[MAX_LEN_32];
    char model[MAX_LEN_32];
    int port;
    char username[MAX_USER_LEN*2];
    char password[MAX_PWD_LEN+1];
    char start_time[MAX_LEN_64];
    char end_time[MAX_LEN_64];
};

/////////////////////////////////////////////////////////////////////////////////
//function

int db_run_sql(const char *db_file, const char *sql);
int db_query_sql(const char *db_file, const char *sql, char *str_result, int size);


/* osd */
int read_osd(const char *pDbFile, struct osd *osd, int chn_id);
int read_osd_name(const char *pDbFile, char *name, int chn_id);
int write_osd(const char *pDbFile, struct osd *osd);
int read_osds(const char *pDbFile, struct osd osds[], int *pCount);
int insert_osd(const char *pDbFile, struct osd *osd);

/* motion */
int read_motion(const char *pDbFile, struct motion *move, int id);
int read_motions(const char *pDbFile, struct motion moves[], int *pCnt);
int write_motion(const char *pDbFile, struct motion *move);
int read_motion_schedule(const char *pDbFile, struct motion_schedule  *motionSchedule, int chn_id);
int delete_motion_schedule(const char *pDbFile, int chn_id);
int write_motion_schedule(const char *pDbFile, struct motion_schedule *schedule, int chn_id);
int insert_motion(const char *pDbFile, struct motion *move);
//motion add
int read_motion_audible_schedule(const char *pDbFile, struct motion_schedule  *motionSchedule, int chn_id);
int delete_motion_audible_schedule(const char *pDbFile, int chn_id);
int write_motion_audible_schedule(const char *pDbFile, struct motion_schedule *schedule, int chn_id);


/* alarm out */
int read_alarm_out(const char *pDbFile, struct alarm_out *alarm, int alarm_id);
int read_alarm_outs(const char *pDbFile, struct alarm_out alarms[], int *pCount);
int write_alarm_out(const char *pDbFile, struct alarm_out *alarm);
int read_alarm_out_schedule(const char *pDbFile, struct alarm_out_schedule *schedule, int alarm_id);
int write_alarm_out_schedule(const char *pDbFile, struct alarm_out_schedule *schedule, int alarm_id);

/* video loss */
int read_video_lost(const char *pDbFile, struct video_loss *videolost, int chn_id);
int read_video_losts(const char *pDbFile, struct video_loss videolosts[], int *pCnt);
int write_video_lost(const char *pDbFile, struct video_loss *videolost);
int read_video_loss_schedule(const char *pDbFile, struct video_loss_schedule *videolostSchedule, int chn_id);
int write_video_loss_schedule(const char *pDbFile, struct video_loss_schedule *schedule, int chn_id);
int insert_video_lost(const char *pDbFile, struct video_loss *videolost);
//video loss add
int read_videoloss_audible_schedule(const char *pDbFile, struct video_loss_schedule *videolostSchedule, int chn_id);
int write_videoloss_audible_schedule(const char *pDbFile, struct video_loss_schedule *schedule, int chn_id);


/* camera */
int read_cameras_type(const char *pDbFile, int types[]);
int read_camera(const char *pDbFile,  struct camera *cam, int chn_id);
int read_cameras(const char *pDbFile, struct camera cams[], int *cnt);
int write_camera_codec(const char *pDbFile, struct camera *cam);
int write_camera(const char *pDbFile, struct camera *cam);
int write_cameras(const char *pDbFile, struct camera cams[]);
int insert_camera(const char *pDbFile, struct camera *cam);
int disable_ip_camera(const char *pDbFile, int nCamId);
int clear_cameras(const char *pDbFile);



/* ip camera */
/*
int read_ip_cameras(const char *pDbFile, struct camera ipCams[], int *pCnt);
int write_ip_camera(const char *pDbFile, struct camera *ipCam);
*/

/* record */
int read_record(const char *pDbFile, struct record *record, int chn_id);
int read_records(const char *pDbFile, struct record records[], int *pCnt);
int read_records_ex(const char *pDbFile, struct record records[], long long changeFlag);
int write_record(const char *pDbFile, struct record *record);
int write_records(const char *pDbFile,struct record records[],long long changeFlag);
int read_record_schedule(const char *pDbFile, struct record_schedule *schedule, int chn_id);
int write_record_schedule(const char *pDbFile, struct record_schedule *schedule, int chn_id);
int reset_record_schedule(const char *pDbFile, int chn_id);
int write_record_prev_post(const char *pDbFile, struct record *record);
int insert_record(const char *pDbFile, struct record *record);

/* holiday */
int read_holidays(const char *pDbFile, struct holiday holidays[], int *pCnt);
int write_holiday(const char *pDbFile, struct holiday *holiday);

/* ptz port */
int read_ptz_port(const char *pDbFile, struct ptz_port *port, int chn_id);
int write_ptz_port(const char *pDbFile, struct ptz_port *port);
int read_ptz_ports(const char *pDbFile, struct ptz_port ports[], int *pCnt);

/* alarm in */
int read_alarm_in(const char *pDbFile, struct alarm_in *alarm, int alarm_id);
int read_alarm_ins(const char *pDbFile, struct alarm_in alarms[], int *pCnt);
int write_alarm_in(const char *pDbFile, struct alarm_in *alarm);
int read_alarm_in_schedule(const char *pDbFile, struct alarm_in_schedule *schedule, int alarm_id);
int write_alarm_in_schedule(const char *pDbFile, struct alarm_in_schedule *schedule, int alarm_id);
//alarm in add
int read_alarmin_audible_schedule(const char *pDbFile, struct alarm_in_schedule *schedule, int alarm_id);
int write_alarmin_audible_schedule(const char *pDbFile, struct alarm_in_schedule *schedule, int alarm_id);
int read_alarmin_ptz_schedule(const char *pDbFile, struct alarm_in_schedule *schedule, int alarm_id);
int write_alarmin_ptz_schedule(const char *pDbFile, struct alarm_in_schedule *schedule, int alarm_id);

/* user */
int read_user_by_name(const char *pDbFile, struct db_user *user, char *username);
int read_users(const char *pDbFile, struct db_user users[], int *pCnt);
int add_user(const char *pDbFile, struct db_user *user);
int write_user(const char *pDbFile, struct db_user *user);
int read_user_count(const char *pDbFile, int *pCnt);
int read_user(const char *pDbFile, struct db_user *user, int id);

/* display */
int read_display(const char *pDbFile, struct display *display);
int read_display_oem(const char *pDbFile, struct display *display);
int write_display(const char *pDbFile, struct display *display);

/* spot */
int read_spots(const char *pDbFile, struct spot spots[], int *pCnt);
int write_spot(const char *pDbFile, struct spot *spot);

/* time */
int read_time(const char *pDbFile, struct time *times);
int write_time(const char *pDbFile, struct time *times);

/* network */
int read_network_mode(const char *pDbFile, int *pMode);
int read_network_bond(const char *pDbFile, struct network *net);
int read_network_muti(const char *pDbFile, struct network *net);
int write_network_bond(const char *pDbFile, struct network *net);
int write_network_muti(const char *pDbFile, struct network *net);
int read_network(const char *pDbFile, struct network *net);
int write_network(const char *pDbFile, struct network *net);

/* pppoe */
int read_pppoe(const char *pDbFile, struct pppoe *pppoe);
int write_pppoe(const char *pDbFile, struct pppoe *pppoe);

/* ddns */
int read_ddns(const char *pDbFile, struct ddns *ddns);
int write_ddns(const char *pDbFile, struct ddns *ddns);

/* upnp */
int read_upnp(const char *pDbFile, struct upnp *upnp);
int write_upnp(const char *pDbFile, struct upnp *upnp);

/* email */
int read_email(const char *pDbFile, struct email *email);
int write_email(const char *pDbFile, struct email *email);

/* network_more */
int read_port(const char *pDbFile, struct network_more *more);
int write_port(const char *pDbFile, struct network_more *more);

/* snmp */
int read_snmp(const char *pDbFile, struct snmp *snmp);
int write_snmp(const char *pDbFile, struct snmp *snmp);

/* audio in */
int read_audio_ins(const char *pDbFile, struct audio_in audioins[MAX_AUDIOIN], int *pCnt);
int write_audio_in(const char *pDbFile, struct audio_in *audioin);

/* layout */
int read_layout(const char *pDbFile, struct layout *layout);
int write_layout(const char *pDbFile, struct layout *layout);

/* ptz speed */
int read_ptz_speeds(const char *pDbFile, struct ptz_speed ptzspeeds[MAX_CAMERA], int *pCnt);
int read_ptz_speed(const char *pDbFile, struct ptz_speed *ptzspeed, int chn_id);
int write_ptz_speed(const char *pDbFile, struct ptz_speed *ptzspeed);

/* storage */
int read_storages(const char *pDbFile, struct storage storages[SATA_MAX], int *pCnt);
int read_storage(const char *pDbFile, struct storage *storage, int port_id);
int write_storage(const char *pDbFile, struct storage *storage);

/* ptz tour */
int read_ptz_tour(const char *pDbFile, struct ptz_tour tour[TOUR_MAX], int ptz_id);
int write_ptz_tour(const char *pDbFile, struct ptz_tour tour[TOUR_MAX], int ptz_id);

/* key-value */
int get_param_value(const char *pDbFile, const char *key, char *value, int value_len, const char *sdefault);
int get_param_int(const char *pDbFile, const char *key, int defvalue);
int set_param_value(const char *pDbFile, const char *key, char *value);
int set_param_int(const char *pDbFile, const char *key, int value);
int check_param_key(const char *pDbFile, const char *key);
int add_param_value(const char *pDbFile, const char *key, char *value);
int add_param_int(const char *pDbFile, const char *key, int value);
//params_oem
int get_param_oem_value(const char *pDbFile, const char *key, char *value, int value_len, const char *sdefault);
int get_param_oem_int(const char *pDbFile, const char *key, int defvalue);
int set_param_oem_value(const char *pDbFile, const char *key, char *value);
int set_param_oem_int(const char *pDbFile, const char *key, int value);
int check_param_oem_key(const char *pDbFile, const char *key);
int add_param_oem_value(const char *pDbFile, const char *key, char *value);
int add_param_oem_int(const char *pDbFile, const char *key, int value);

/* ipc protocol */
int read_ipc_protocols(const char *pDbFile, struct ipc_protocol **protocols, int *pCnt);
int read_ipc_protocol(const char *pDbFile, struct ipc_protocol *protocol, int pro_id);
int write_ipc_protocol(const char *pDbFile, struct ipc_protocol *protocol);

/* mosaic */
int read_mosaic(const char *pDbFile, struct mosaic *mosaic);
int read_mosaics(const char *pDbFile, struct mosaic mosaic[LAYOUT_MODE_MAX], int *pCnt);
int write_mosaic(const char *pDbFile, struct mosaic *mosaic);

int read_disk_maintain_info(const char *pDbFile, struct disk_maintain_info *info);
int write_disk_maintain_info(const char *pDbFile, struct disk_maintain_info *info);

int read_p2p_info(const char *pDbFile, struct p2p_info *info);
int write_p2p_info(const char *pDbFile, struct p2p_info *info);

int read_privacy_masks(const char *pDbFile, struct privacy_mask mask[], int *pCnt, DB_TYPE o_flag);
int read_privacy_mask(const char *pDbFile, struct privacy_mask *mask, int chn_id);
int write_privacy_mask(const char *pDbFile, struct privacy_mask *mask);
int insert_privacy_mask(const char *pDbFile, struct privacy_mask *mask);

int read_alarm_pids(const char *pDbFile, struct alarm_pid **pid, int *pCnt);
int read_alarm_pid(const char *pDbFile, struct alarm_pid *pid);
int write_alarm_pid(const char *pDbFile, struct alarm_pid *pid);
int update_alarm_pid(const char *pDbFile, struct alarm_pid *pid);

/////////////////////////////////////////////////////////////////////////////////////////////////////
int read_trigger_alarms(const char *pDbFile, struct trigger_alarms *pa);
int write_trigger_alarms(const char *pDbFile, const struct trigger_alarms *pa);
/////////////////////////////////////////////////////////////////////////////////////////////////////

/* camera io */
int read_cameraios(const char *pDbFile, struct camera_io cio[], int *cnt);
int read_cameraio(const char *pDbFile, struct camera_io *pio, int chanid);
int write_cameraio(const char *pDbFile, const struct camera_io *pio);
int insert_cameraio(const char *pDbFile, const struct camera_io *pio);

/* voluem detect */
int read_volumedetects(const char *pDbFile, struct volume_detect vd[], int *cnt);
int read_volumedetect(const char *pDbFile, struct volume_detect *pvd, int chanid);
int write_volumedetect(const char *pDbFile, const struct volume_detect *pvd);
int insert_volume_detect(const char *pDbFile, const struct volume_detect *pvd);

/* ipc protocol */
int read_ipc_protocols(const char *pDbFile, struct ipc_protocol **protocols, int *pCnt);
int read_ipc_protocol(const char *pDbFile, struct ipc_protocol *protocol, int pro_id);
int write_ipc_protocol(const char *pDbFile, struct ipc_protocol *protocol);
int insert_ipc_protocol(const char *pDbFile, struct ipc_protocol *protocol);
int delete_ipc_protocols(const char *pDbFile);

/* ipc model */
int read_ipc_models(const char *pDbFile, struct ipc_model **models, int *pCnt);
int read_ipc_models_by_protocol(const char *pDbFile, struct ipc_model **models, int pro_id, int *pCnt);
int read_ipc_model(const char *pDbFile, struct ipc_model *model, int mod_id);
int read_ipc_default_model(const char *pDbFile, struct ipc_model *model, int pro_id);
int write_ipc_model(const char *pDbFile, struct ipc_model *model);
int insert_ipc_model(const char *pDbFile, struct ipc_model *model);
int delete_ipc_models(const char *pDbFile);
void ipc_destroy(void *ptr);

//bruce.milesight add for param device
int db_get_device(const char *pDbFile, struct device_info *device);
int db_set_device(const char *pDbFile, struct device_info *device);
int db_get_device_oem(const char *pDbFile, struct device_info *device);
int db_set_device_oem(const char *pDbFile, struct device_info *device);

//bruce.milesight add for tz db
int db_get_tz_filename(const char *pDbFile, const char *tz, char *tzname, int tzname_size, char *tzfname, int tzfname_size);

int write_layout1_chn(const char *pDbFile, int chnid);
int read_layout1_chn(const char *pDbFile, int *chnid);

//#########for default DB#########//
int db_read_user_by_name(const char *pDbFile, struct db_user *user, char *username);

int read_users_oem(const char *pDbFile, struct db_user_oem users[], int *pCnt);

int write_sub_layout1_chn(const char *pDbFile, int chnid);
int read_sub_layout1_chn(const char *pDbFile, int *chnid);

int read_poe_cameras(const char *pDbFile, struct camera cams[], int *cnt);
int write_autoreboot_conf(const char *pDbFile, reboot_conf* conf);
int read_autoreboot_conf(const char *pDbFile, reboot_conf* conf);

//7.0.6
int read_privacy_mask_by_areaid(const char *pDbFile, struct privacy_mask *mask, int chn_id, int area_id);
int write_privacy_mask_by_areaid(const char *pDbFile, struct privacy_mask *mask, int area_id);

int read_motion_email_schedule(const char *pDbFile, struct motion_schedule  *motionSchedule, int chn_id);
int write_motion_email_schedule(const char *pDbFile, struct motion_schedule *schedule, int chn_id);	
int read_videoloss_email_schedule(const char *pDbFile, struct video_loss_schedule  *schedule, int chn_id);
int write_videoloss_email_schedule(const char *pDbFile, struct video_loss_schedule  *schedule, int chn_id);
int read_alarmin_email_schedule(const char *pDbFile, struct alarm_in_schedule *schedule, int chn_id);
int write_alarmin_email_schedule(const char *pDbFile, struct alarm_in_schedule *schedule, int chn_id);
int read_motion_popup_schedule(const char *pDbFile, struct motion_schedule  *motionSchedule, int chn_id);
int write_motion_popup_schedule(const char *pDbFile, struct motion_schedule *schedule, int chn_id);	


int read_alarm_in_ex(const char *pDbFile, struct alarm_in *alarm, int alarm_id);
int read_alarm_ins_ex(const char *pDbFile, struct alarm_in alarms[], int *pCnt);
int write_alarm_in_ex(const char *pDbFile, struct alarm_in *alarm);

int check_table_key(const char *pDbFile, char *table, const char *key);

int read_disks(const char *pDbFile, struct diskInfo disks[], int *pCnt);
int read_disk(const char *pDbFile, struct diskInfo *disk, int portId);
int write_disk(const char *pDbFile, struct diskInfo *disk);
int insert_disk(const char* pDbFile, struct diskInfo*diskInfo);

int read_groups(const char *pDbFile, struct groupInfo groups[], int *pCnt);
int write_group_st(const char *pDbFile, struct groupInfo *group);

int write_ssl_params(const char *pDbFile, struct cert_info *cert);
int read_ssl_params(const char *pDbFile, struct cert_info *cert,int type);

int write_encrypted_list(const char *pDbFile, struct squestion *sqa, int sorder);
int read_encrypted_list(const char *pDbFile, struct squestion sqas[]);
int read_smart_event_effective_schedule(const char *pDbFile, struct smart_event_schedule  *Schedule, int chn_id, SMART_EVENT_TYPE type);
int write_smart_event_effective_schedule(const char *pDbFile, struct smart_event_schedule *schedule, int chn_id, SMART_EVENT_TYPE type);

int read_smart_event_audible_schedule(const char *pDbFile, struct smart_event_schedule  *Schedule, int chn_id, SMART_EVENT_TYPE type);
int write_smart_event_audible_schedule(const char *pDbFile, struct smart_event_schedule *schedule, int chn_id, SMART_EVENT_TYPE type);

int read_smart_event_mail_schedule(const char *pDbFile, struct smart_event_schedule  *Schedule, int chn_id, SMART_EVENT_TYPE type);
int write_smart_event_mail_schedule(const char *pDbFile, struct smart_event_schedule *schedule, int chn_id, SMART_EVENT_TYPE type);

int read_smart_event_popup_schedule(const char *pDbFile, struct smart_event_schedule  *Schedule, int chn_id, SMART_EVENT_TYPE type);
int write_smart_event_popup_schedule(const char *pDbFile, struct smart_event_schedule *schedule, int chn_id, SMART_EVENT_TYPE type);

int read_smart_event(const char *pDbFile, struct smart_event *smartevent, int id, SMART_EVENT_TYPE type);
int write_smart_event(const char *pDbFile, struct smart_event *smartevent, SMART_EVENT_TYPE type);


int read_motion_effective_schedule(const char *pDbFile, struct smart_event_schedule  *Schedule, int chn_id);
int write_motion_effective_schedule(const char *pDbFile, struct smart_event_schedule *schedule, int chn_id);

int read_smart_events(const char *pDbFile, struct smart_event *smartevent, int chnid);
int read_smart_event_effective_schedules(const char *pDbFile, struct smart_event_schedule *Schedule, int chn_id);
int read_smart_event_audible_schedules(const char *pDbFile, struct smart_event_schedule *Schedule, int chn_id);
int read_smart_event_mail_schedules(const char *pDbFile, struct smart_event_schedule *Schedule, int chn_id);
int read_smart_event_popup_schedules(const char *pDbFile, struct smart_event_schedule *Schedule, int chn_id);

int read_smart_event_all_chns(const char *pDbFile, struct smart_event *smartevent[], int allChn);
int read_smart_event_effective_schedules_all_chns(const char *pDbFile, struct smart_event_schedule *Schedule[], int allChn);
int read_smart_event_audible_schedules_all_chns(const char *pDbFile, struct smart_event_schedule *Schedule[], int allChn);
int read_smart_event_mail_schedules_all_chns(const char *pDbFile, struct smart_event_schedule *Schedule[], int allChn);
int read_smart_event_popup_schedules_all_chns(const char *pDbFile, struct smart_event_schedule *Schedule[], int allChn);
int write_smart_event_effective_schedule_default(const char *pDbFile, int chn_id);

int read_failover(const char *pDbFile, struct failover_list *failover, int id);
int read_failovers(const char *pDbFile, struct failover_list failovers[], int *cnt);
int write_failover(const char *pDbFile, struct failover_list *failover);


#ifdef __cplusplus
}
#endif

#endif

#if 0

1.update table mosaic table.set channels blog.
2.init table params default value.

#endif
