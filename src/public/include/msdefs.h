#ifndef _MS_DEFS_H_
#define _MS_DEFS_H_
#include "msstd.h"
//#include "vapi/vapi.h"

#define THTEAD_SCHED_RR_PRI			60
#define URL_ID_LIVE_MAIN			100
#define URL_ID_LIVE_SUB				400
#define URL_ID_PLAYBACK				800
#define MAX_STREAM_NUM 				2
#define MAX_FRAME 					60
#define MAX_LEN_16					16
#define MAX_LEN_32 					32
#define MAX_LEN_64 					64
#define MAX_LEN_128 				128
#define MAX_LEN_256 				256
#define MAX_LEN_512 				512
#define MAX_LEN_1024				1024

#define RTSP_MAX_USER 				(20)  		//10 -> 20 
#define MAX_USER_LEN 				32
#define MAX_PWD_LEN 				32
#define	MAX_PSNAME_LEN				31
#define MAX_MAIL_RECORD_CNT 		(360)

#define MAX_FAILOVER				(32)

#define MAX_CAMERA					(64)
#define MAX_SMART_CTRL_CNT 			(64)
#define MAX_SMART_PLAYBACK_CNT 		(MAX_CAMERA)
#define MAX_RETRIEVE_MEM_SIZE 		(4*1024*1024)
#define MAX_SEARCH_REC_CNT 			(100)

#define PARAMS_SDK_VERSION			"2.3.06"
#define PARAMS_P2P_SDK_VERSION		"2.3.05"

#define FMT_DONE_FLAG				"reco.info"
#define MS_NO_REQS					"no reqs"

#define MS_SH_PATH 					"/opt/app"
#define MS_ETC_PATH 				"/opt/app/etc"
#define MS_EXE_PATH 				"/opt/app/bin"
#define MS_WEB_PATH 				"/opt/app/www"
#define MS_CONF_DIR 				"/mnt/nand/app"
#define MS_ETC_DIR					"/mnt/nand/etc"
#define MS_NAND_DIR					"/mnt/nand"
#define MS_OEM_DIR 					"/mnt/nand2"
#define MS_RESERVE_DIR				"/mnt/nand3"
#define PATH_TEMP_FILE				"/tmp"
#define MS_CORE_PID 				"/tmp/mscore"
#define MS_CLI_PID					"/tmp/mscli"
#define MS_GUI_PID					"/tmp/gui"
#define MS_BOA_PID 					"/tmp/boa"

#define MS_RESET_FLAG_FILE 			"eraseall"
#define MS_RESET_FLAG_IP_FILE 		"eraseip"
#define MS_RESET_FLAG_USER_FILE 	"eraseuser"
#define PARA_FILE_PATH 				"/msdb.sql"
#define LOAD_PARA_FILE_PATH 		"/tmp/loadpara.txt"
#define FAILOVER_SLAVE_INIT         "slaveinit"
//#define KEEP_CONFIG_FILE 			"loadpara.txt"
#define WATCHDOG_CONF_NAME 			"watchdog.conf"
#define WEB_DOWNLOAD_NAME_FILE  	"/tmp/.export_down_file"
#define GUI_LANG_INI				"/mnt/nand/etc/language.ini"
#define MS_KEEP_CONF_FILE 			"keep_conf"
#define MS_KEEP_USER_CONF_FILE 		"keep_user_conf"
#define MS_KEEP_ENCRYPTED_CONF_FILE "keep_encrypted_conf"
#define MS_MAC0_CONF				"/mnt/nand/etc/mac.conf"

#define MS_DDNS_DOMAIN 				"ddns.milesight.com"
#define UPDATEQUEST 				"/cgi-bin/ddns/1002"
#define DELETEQUEST 				"/cgi-bin/ddns/1005"
#define SERVERDEFAULTUSER 			"milesight"
#define SERVERDEFAULTPWD 			"milesight1612"
#define SERVERHTTPPORT 				"80"
#define SERVERGETPUBLICIP 			"ip1.dynupdate.no-ip.com"
#define SERVERGETPUBLICIP2 			"www.ip138.com"

#define TOOL_NAME			 		"update"
#define MS_UPDATE_DB_FILE			"/mnt/nand/updb.cfg"

#define RECORD_TYPE_ALARM			"Alarm Input"
#define RECORD_TYPE_MOTION			"CH"

#define CODECTYPE_H264              0
#define CODECTYPE_MPEG4             1
#define CODECTYPE_MJPEG             2
#define CODECTYPE_H265				3

//record frame rate
#define BITRATE_TYPE_CBR            0
#define BITRATE_TYPE_VBR            1

#define IFRAME_INTERVAL_30          0
#define IFRAME_INTERVAL_15          1
#define IFRAME_INTERVAL_10          2
#define IFRAME_INTERVAL_5           3
#define IFRAME_INTERVAL_1           4

#define FRAMERATE_30_25             0   //NTSC 30, PAL 25
#define FRAMERATE_15_13             1   //NTSC 15, PAL 13
#define FRAMERATE_08_06             2   //NTSC 8,  PAL 6
#define FRAMERATE_04_03             3   //NTSC 4,  PAL 3

#define CODECTYPE_H264              0
#define CODECTYPE_MPEG4             1
#define CODECTYPE_MJPEG             2
#define CODECTYPE_H265				3

#define RESOLUTION_D1         		0
#define RESOLUTION_CIF            	1
#define RESOLUTION_HALFD1           2
#define RESOLUTION_QCIF             3
#define RESOLUTION_720P				4


//db start
//user
#define	MAX_USER					10
//MOTION DETECT
#define MOTION_W_CELL               23
#define MOTION_H_CELL               13
#define MOTION_PAL_INC              3
#define MAX_MOTION_CELL             MOTION_W_CELL*(MOTION_H_CELL+MOTION_PAL_INC) //23*(13+3)
//email
#define EMAIL_RECEIVER_NUM          3
#define MAX_HOLIDAY                 32
#define MAX_MASK_AREA_NUM			4
//HDD SATA NUM
#define SATA_MAX					16
#define RAID_MAX					5
#define SCSI_N04_MAX				2
#define SCSI_N03_MAX				4
#define SCSI_N01_MAX				1

//FOR MSFS DISK INFO
#define MAX_MSDK_GROUP_NUM  (16)
#define MAX_MSDK_LOCAL_NUM  (16)
#define MAX_MSDK_NET_NUM    (8)
#define MAX_MSDK_ESATA_NUM  (1)
#define MAX_MSDK_RAID       (8)
#define MAX_USB_NUM			(8)
#define MAX_MSDK_PORT_NUM   (MAX_MSDK_LOCAL_NUM+MAX_MSDK_NET_NUM+MAX_MSDK_ESATA_NUM+MAX_MSDK_RAID)
#define MAX_MSDK_NUM   		((MAX_MSDK_PORT_NUM<36)?(36):(MAX_MSDK_PORT_NUM))

//PTZ
#define PATTERN_MAX                 4
#define TOUR_MAX                    8
#define KEY_MAX                    	48
#define PRESET_MAX					255

//fisheye stream
#define STREAM_MAX			5

//SPOT
#define DVR_SPOT_IN_CH				16
#define DVR_SPOT_OUT_CH				4
//AUDIO
#define MAX_AUDIOIN                 16
#define AUDIO_CODEC_G711            0
#define AUDIO_CODEC_AAC             1
#define AUDIO_CODEC_G711_MULAW      2
#define AUDIO_CODEC_G711_ALAW       3
//DIO
#define MAX_ALARM_IN                16
#define ALARM_IN_TYPE_NO            0
#define ALARM_OUT_TYPE_NC  	        1

#define MAX_ALARM_OUT               4
#define ALARM_IN_TYPE_NO            0
#define ALARM_OUT_TYPE_NC           1

//end dbstart

#define CAMERA_TYPE_INTERNAL        0
#define CAMERA_TYPE_IPNC            1

#define PTZ_CONN_TYPE_INTERNAL		1
#define PTZ_CONN_TYPE_ONVIF			0

#define MAX_PORT_NUM    16

#define POE_TOTAL_POWER_4 			(30.0)
#define POE_TOTAL_POWER_8 			(105.0)
#define POE_TOTAL_POWER_16 			(185.0)

#define MAX_POE_TOTAL_POWER_4  		(45.0)
#define MAX_POE_TOTAL_POWER_8 	    (135.0)
#define MAX_POE_TOTAL_POWER_16 		(215.0)

#define	MAX_PRESET_CNT	255


#define HDD_LOCAL	        DISK_TYPE_LOCAL
#define HDD_CD		        1
#define HDD_ESATA	        2
#define HDD_USB		        DISK_TYPE_USB
#define HDD_NET		        4
#define HDD_RAID_DISK       5
#define HDD_RAID            6
#define HDD_GLOBAL_SPARE    7
#define HDD_LOCAL_SPARE     8

#define MAX_SQA_CNT 		3
#define BIT1_MARK           (1ULL)
#define BIT1_LSHFT(shift)           (BIT1_MARK << (shift))

//LOG
#define LOG_MAX_TURN_SEND   100 // ((SOCKET_DATA_MAX_SIZE / LOG_DATA_SIZE) - 1)

#if defined(_HI3536_)
	#define MS_PLATFORM "3536" 
#elif defined(_HI3798_)
	#define MS_PLATFORM "3798"  
#endif

typedef enum 
{
    NETMODE_MULTI = 0,
    NETMODE_LOADBALANCE,
    NETMODE_BACKUP,
    NETMODE_MAX,
}NETMODE;

typedef enum
{
    TRANSPROTOCOL_UDP,
    TRANSPROTOCOL_TCP,
    TRANSPROTOCOL_MAX,
}TRANSPROTOCOL;

enum frm_type
{
	FT_IFRAME=0, 
	FT_PFRAME,
};

enum strm_type
{
	ST_NONE=0, 
	ST_VIDEO, 
	ST_AUDIO, 
	ST_DATA, 
	MAX_ST, 
};

enum ms_res_level
{
    MS_RES_D1 = 1,
    MS_RES_720P,
    MS_RES_1080P,
    MS_RES_3M,
    MS_RES_5M,
};

enum record_event_type
{
    LIB_EVENT_REC_NONE       = 0,
    LIB_EVENT_REC_CONT       = (1 << 0),
    LIB_EVENT_REC_MOT        = (1 << 1),
    LIB_EVENT_REC_IO         = (1 << 2),
    LIB_EVENT_REC_VLOSS      = (1 << 3),
    LIB_EVENT_REC_EMG        = (1 << 4),
    LIB_EVENT_REC_MOT_OR_IO  = (1 << 17),
    LIB_EVENT_REC_MOT_AND_IO = (1 << 18),
};

enum stream_source
{
	SST_NONE = 0,	
	SST_ENC_VIDEO		= 1 << 0,
	SST_ENC_MOTION		= 1 << 1,
	SST_ENC_SNAPSHOT	= 1 << 2,
	SST_LOCAL_PB		= 1 << 3,
	SST_REMOTE_PB		= 1 << 4,
	SST_IPC_STREAM		= 1 << 5,
    SST_DATA_STREAM 	= 1 << 6,
    SST_LOCAL_AUDIO     = 1 << 7,
    SST_EVENT_STREAM    = 1 << 8,
    SST_REMOTE_TRANSFER = 1 << 9,
    SST_IPC_ANR         = 1 << 10,
};

typedef enum stream_type
{
	STREAM_TYPE_MAINSTREAM,
	STREAM_TYPE_SUBSTREAM,
 	STREAM_TYPE_ALLSTREAM,
	STREAM_TYPE_MAX,
	STREAM_TYPE_NONE,
}STREAM_TYPE;

typedef enum
{
 	IPC_PROTOCOL_ONVIF = 0,
	IPC_PROTOCOL_RTSP,
	IPC_PROTOCOL_MILESIGHT,
	IPC_PROTOCOL_CANON,
	IPC_PROTOCOL_NESA,
	IPC_PROTOCOL_PSIA,
	IPC_PROTOCOL_ALPHAFINITY,
   IPC_PROTOCOL_MAX = IPC_PROTOCOL_ALPHAFINITY
} IPC_PROTOCOL;


//schedule record action
enum schedule_action
{
    NONE = 0,
    TIMING_RECORD,
    MOTION_RECORD,
    ALARM_RECORD,
    MOTION_OR_ALARM_RECORD,
    MOTION_AND_ALARM_RECORD,
    ALARMIN_ACTION,
    ALARMOUT_ACTION,
    MOTION_ACTION,
    VIDEOLOSS_ACTION,
    SMART_EVT_RECORD, //10 VCA
    EVENT_RECORD
};

enum record_mode
{
    MODE_NO_RECORD = 0,
    MODE_ALWAYS_REC,
    MODE_SCHED_REC,
};
enum event_mode{
    MODE_DISABLE = 0,
    MODE_ALWAYS,
    MODE_BY_SCHED,
};
enum except_event
{
    EXCEPT_NETWORK_DISCONN = 0,
    EXCEPT_DISK_FULL,
    EXCEPT_RECORD_FAIL,
    EXCEPT_DISK_FAIL,
    EXCEPT_DISK_NO_FORMAT,
    EXCEPT_NO_DISK,
    EXCEPT_COUNT,
};

enum push_event
{
    PUSH_EVENT_DISK_FULL = 1,
    PUSH_EVENT_DISK_UNINIT,
    PUSH_EVENT_DISK_FAIL,
    PUSH_EVENT_DISK_DEL,
    PUSH_EVENT_DISK_OFFLINE,
    PUSH_EVENT_DISK_NORMAL,
    PUSH_EVENT_NO_DISK,
    PUSH_EVENT_HAVE_DISK,
    PUSH_EVENT_RECORD_FAIL,
    PUSH_EVENT_RECORD_START,
    PUSH_EVENT_RECORD_STOP,
    PUSH_EVENT_CAM_DISCONNECT,
    PUSH_EVENT_NET_UP,
    PUSH_EVENT_NET_DOWN,
    PUSH_EVENT_UDISK_DOWN,
};


//OSD DATE FORMAT
enum osd_date_fmt{
        YYYY_MM_DD = 0,
        YYYY_DD_MM,
        MM_DD_YYYY,
        DD_MM_YYYY,
};
enum qt_date_fmt{
        OSD_DATEFORMAT_MMDDYYYY_AP = 0,
        OSD_DATEFORMAT_DDMMYYYY_AP,
        OSD_DATEFORMAT_YYYYDDMM_AP,
        OSD_DATEFORMAT_YYYYMMDD_AP,
        OSD_DATEFORMAT_MMDDYYYY,
        OSD_DATEFORMAT_DDMMYYYY,
        OSD_DATEFORMAT_YYYYDDMM,
        OSD_DATEFORMAT_YYYYMMDD,
        OSD_DATEFORMAT_MAX
};
enum others_type{
	VIDEO_LOST_TYPE = 0,
	MOTION_TYPE,
	ALARM_IN_TYPE
};

////////@david need to do  Start//////////
typedef enum smart_health_status {
	HDD_HEALTH_PASSED,
	HDD_HEALTH_FAILED
}HDD_HEALTH_STATUS;

typedef enum disk_state{
    DISK_UNFORMAT = 0, ///< unformatted
    DISK_FORMATING, ///< formatting
    DISK_FORMATTED, ///< formatted
    MAX_STATE
}DISK_STATE;
////////@david need to do End//////////


//p2p 
typedef enum resp_errcode
{
	ERR_OK = 0,
	ERR_UNAUTH,
	ERR_NOTFOUND,
	ERR_P2P_NETWORK,
	ERR_UNKNOWN,
	ERR_MAC_FORMAT,
	ERR_AUTH_FAILED,
	ERR_NO_UUID,
	ERR_P2P_INIT_FAILED,
	ERR_P2P_LOGIN_FAILED
}P2P_STATUS;

typedef enum _device_type{
	IPC = 0,
	NVR,
	APP
}DEVICE_TYPE;

//user
#define MAX_USER                                    10
typedef enum {
        USERLEVEL_ADMIN = 0,
        USERLEVEL_OPERATOR,
        USERLEVEL_USER,
        USERLEVEL_LOCAL,
        USERLEVEL_MAX
}USER_LEVEL;
	
	
typedef enum osd_stream_type{
	OSD_ALL_STREAM = 0,
	OSD_MAIN_STREAM,
	OSD_SUB_STREAM,		
	OSD_THD_STREAM
}OSD_STREAM_TYPE;

typedef enum IpcType
{
	OTHER_IPC=0,
	HISI_IPC,
	AMBA_IPC,
	TI_IPC,
	MS_SPEEDOME,
	HISI_4MPIPC,
	FISHEYE_IPC,
}MS_IPC_TYPE;

typedef enum db_type{
	DB_OLD = 0,
	DB_NEW,
}DB_TYPE;

typedef enum  ipc_conn_res
{
	IPC_OK = 0,	
	IPC_NETWORK_ERR,
	IPC_INVALID_USER,
	IPC_UNKNOWN_ERR,
	IPC_PROTO_NOT_SUPPORT,
	IPC_OUT_LIMIT_BANDWIDTH
}IPC_CONN_RES;

typedef enum
{
	PLAY_SPEED_0_125X = -3,
	PLAY_SPEED_0_25X = -2,
	PLAY_SPEED_0_5X = -1,
	PLAY_SPEED_1X = 0,
	PLAY_SPEED_2X,
	PLAY_SPEED_4X,
	PLAY_SPEED_8X,
	PLAY_SPEED_16X,
	PLAY_SPEED_32X,
	PLAY_SPEED_64X,
	PLAY_SPEED_128X
}PLAY_SPEED;

enum CameraType
{	
	RTSP_CONNECT=-3,
	DICONNECT=-2,
};

typedef enum Smart_Event
{	
	REGIONIN = 0,
	REGIONOUT,
	ADVANCED_MOTION,
	TAMPER,
	LINECROSS,
	LOITERING,
	HUMAN,
	PEOPLE_CNT,
	MAX_SMART_EVENT
}SMART_EVENT_TYPE;

#define USERACCESS_PLAYBACK                         (1<<0)
#define USERACCESS_DISPLAY                          (1<<1)
#define USERACCESS_CAMERA                           (1<<2)
#define USERACCESS_RECORD                           (1<<3)
#define USERACCESS_ALARM                            (1<<4)
#define USERACCESS_STATUS                           (1<<5)
#define USERACCESS_BACKUP                           (1<<6)
#define USERACCESS_SYSTEM                           (1<<7)
#define USERACCESS_STORAGE                          (1<<8)

//User Privilege
#define ACCESS_CHANNEL_MANAGE   (1<<0) 
#define ACCESS_PTZ_CTRL         (1<<1) 
#define ACCESS_EMERGENCY_REC    (1<<2) 
#define ACCESS_SNAPSHOT         (1<<3) 
#define ACCESS_RECORD_CFG       (1<<4) 
#define ACCESS_PLAYBACK         (1<<5) 
#define ACCESS_PLAYBACKEXPORT   (1<<6) 
#define ACCESS_EVENT_CFG        (1<<7) 
#define ACCESS_STATE_LOG        (1<<8) 
#define ACCESS_LIVEVIEW_CFG     (1<<9) 
#define ACCESS_SYS_GENERAL      (1<<10)
#define ACCESS_NETWORK_CFG      (1<<11)
#define ACCESS_DISK_MANAGE      (1<<12)
#define ACCESS_HOLIDAY_CFG      (1<<13)
#define ACCESS_SYS_UPGRADE      (1<<14)
#define ACCESS_PROFILE          (1<<15)
#define ACCESS_SHUTDOWN_REBOOT  (1<<16)
#define ACCESS_SYS_MAINTENANCE  (1<<17)
#define ACCESS_SYS_AUTOREBOOT   (1<<18)
#define ACCESS_RETRIEVE         (1<<19)


struct reco_frame
{
	int ch;					//channels
	int strm_type;			//video & audio & data  ST_VIDEO & ST_AUDIO
	int codec_type;			//264&265 | aac & alaw & ulaw
	int stream_from;		//remote & local 
	int stream_format;		//main & sub stream

	//Timestamp
	unsigned long long time_usec;
	unsigned int time_lower;
	unsigned int time_upper;

	//data size
	int size;
	
	//video
	int frame_type;		   // FT_IFRAME &	FT_PFRAME
	int frame_rate;			
	int width;
	int height;
	
	//audio
	int sample;
	int bps;
	int achn;	

	//data buff 
	void* data;

	int decch_changed;
	int sid;
	int sdk_alarm;	
};


//mod_camera
struct search_param
{
	int protocol;
	int reqfrom;
};


//////////////////////////////////////////////////////////////
//bruce.milesight add
enum language
{
    LNG_ENGLISH = 0,
    LNG_CHINESE = 1,
    LNG_CHINESE_TW = 2,
    LNG_MAGYAR = 3,
    LNG_RUSSIAN = 4,
    LNG_FRENCH = 5,
    LNG_POLSKA = 6,    
    LNG_DANISH = 7,
    LNG_ITALIAN = 8,
    LNG_GERMAN = 9,
    LNG_CZECH = 10,
    LNG_JAPANESE = 11,
    LNG_KOREAN = 12,
    LNG_FINNISH = 13,
    LNG_TURKEY = 14,
    LNG_HOLLAND = 15, 
    LNG_HEBREW = 16,
    LNG_ARABIC = 17,
    LNG_ISRAEL = 18,
};

//gui debug start
#define MAX_NAME_LEN               					256
#define MAX_USERNAME_LEN            				64
#define MAX_PASSWORD_CHAR           				33
#define MAX_QUESTION_CHAR           				120
#define MAX_ANSWER_CHAR      	     				120
#define SQA_CUSTOMIZED_NO							12


#define LAYOUTMODE_1                                (1 << 0)
#define LAYOUTMODE_4                                (1 << 1)
#define LAYOUTMODE_8                                (1 << 2)
#define LAYOUTMODE_8_1                              (1 << 3)
#define LAYOUTMODE_9                                (1 << 4)
#define LAYOUTMODE_12                               (1 << 5)
#define LAYOUTMODE_12_1                             (1 << 6)
#define LAYOUTMODE_14                               (1 << 7)
#define LAYOUTMODE_16                               (1 << 8)
#define LAYOUTMODE_25								(1 << 9)
#define LAYOUTMODE_32								(1 << 10)
#define LAYOUTMODE_36								(1 << 11)
#define LAYOUTMODE_ZOOMIN							(1 << 12)
#define LAYOUTMODE_32_2								(1 << 13)

#define LAYOUT_MODE_MAX								(14)

#define IPC_FUNC_SEARCHABLE	(1 << 0)
#define IPC_FUNC_SETPARAMS	(1 << 1)
#define IPC_FUNC_SETIPADDR	(1 << 2)

//prev or post record
#define PREVRECORDDURATION_SEC_MIN                  1
#define PREVRECORDDURATION_SEC_MAX                  10
#define PREVRECORDDURATION_SEC_DEFAULT              10

#define POSTRECORDDURATION_MINUTE_MIN               1
#define POSTRECORDDURATION_MINUTE_MAX               60
#define POSTRECORDDURATION_MINUTE_DEFALUT           3

typedef enum
{
    OUTPUT_RESOLUTION_1080P = 0,
    OUTPUT_RESOLUTION_720P,
    OUTPUT_RESOLUTION_SXGA,
    OUTPUT_RESOLUTION_XGA,
    OUTPUT_RESOLUTION_1080P50,
    OUTPUT_RESOLUTION_2160P30,
    OUTPUT_RESOLUTION_2160P60,
    OUTPUT_RESOLUTION_MAX = OUTPUT_RESOLUTION_2160P60,
} DisplayDcMode_e;

typedef enum 
{
    DISP_PATH_HDMI0 = 0,
    DISP_PATH_HDMI1,
    DISP_PATH_VGA,
    DISP_PATH_SD,
} DisplayPath_e;

typedef enum
{
    DISP_HDMI0_VGA = 0,
    DISP_MAIN_HDMI0,
}DisplayMode;

typedef enum
{
    CONNECT_MANUAL = 0,
    CONNECT_UPNP,

}CONNECT_MODE;

typedef enum{
	POE_NO_CHAN = 0,
	POE_1_CHAN,
	POE_2_CHAN,
	POE_3_CHAN,
	POE_4_CHAN,
	POE_5_CHAN,
	POE_6_CHAN,
	POE_7_CHAN,
	POE_8_CHAN,
	POE_9_CHAN,
	POE_10_CHAN,
	POE_11_CHAN,
	POE_12_CHAN,
	POE_13_CHAN,
	POE_14_CHAN,
	POE_15_CHAN,
	POE_16_CHAN,
}POE_CHN_TYPE;
//gui debug end

#define BACKUP_ERR_NONE 							0
#define BACKUP_ERR_ARGS 							1
#define BACKUP_ERR_CHANNEL 							2
#define BACKUP_ERR_PATH 							3
#define BACKUP_ERR_TIME								4
#define BACKUP_ERR_CREATE							5
#define BACKUP_ERR_INIT								6
#define BACKUP_ERR_WRITE							7
#define BACKUP_ERR_NOSPACE							8
#define BACKUP_ERR_NOMEM							9

//struct define
struct device_info
{
	int devid;
	int max_cameras;
	int max_pb_num;
	int max_disk;
	int max_alarm_in;
	int max_alarm_out;
	int max_audio_in;
	int max_audio_out;
	int max_screen;//3536:two
	int max_hdmi;//3536:two
	int max_vga;
	int max_rs485;
	int camera_layout;//layout mode LAYOUT_NUM
	int max_lan;//network lan(eth0 eth1)
	int def_lang;//the qt and web default language.
	int oem_type;
	int power_key; //shutdown key
	int msddns;  //for oem whether suport ms ddns
	
	char sncode[MAX_LEN_32];	
	char prefix[MAX_LEN_16];// 1 5 7 8 only one bit
	char model[MAX_LEN_32];//MS-N1009,MS-N8032
	char softver[MAX_LEN_32];
	char hardver[MAX_LEN_32];
	char company[MAX_LEN_128];
	char lang[MAX_LEN_32];
	char mac[MAX_LEN_32];
};

//#define MAX_CHANNEL					64//the real decorder 
//#define MAX_DECODE_CHANNEL			128//the max decoder support 128
#define MAX_CAMERA_QT					64//current QT support 32 only
#define MAX_PERFORM_LIMIT				32

//task name
#define THREAD_NAME_RECPROCESS		"recpro"
#define THREAD_NAME_RECORDPOST		"recpost"
#define THREAD_NAME_CHECKCONN		"chkconn"
#define THREAD_PULL_ALARM			"pullalarm"
#define THREAD_NAME_AUDIOPLAY		"audioplay"
#define THREAD_NAME_LIVE_SNAPSHOT	"livesnapshot"
#define THREAD_NAME_PB_SNAPSHOT		"pbsnapshot"
#define	THREAD_NAME_EXPORT			"export"
#define THREAD_NAME_SEARCH			"search"
#define THREAD_NAME_DISK_FORMAT		"format"
#define THREAD_NAME_RAID_REBUILD   	"rebuild"
#define THREAD_NAME_DISK_CHECK		"diskchk"
#define THREAD_NAME_P2P_IOCTL		"p2pioctl"
#define THREAD_NAME_P2P_INIT		"p2p"
#define THREAD_NAME_IRIS			"iris"
#define THREAD_NAME_PUSH_MSG		"pushmsg"
#define THREAD_NAME_SCHEDULE		"schedule"
#define THREAD_NAME_STREAM_SWITCH	"camswitch"
#define THREAD_NAME_POE_DISCOVERY	"poediscovery"
#define THREAD_NAME_PBDATE_SWITCH	"pbdateswitch"
#define THREAD_NAME_DISK_LOG		"disklog"
#define THREAD_NAME_ONVIF			"ovfhdlupdate"
#define THREAD_AUTO_REBOOT			"autoreboot"
#define THREAD_NAME_DDNS			"msddns"
#define THREAD_NAME_POWER_ADJUST	"poweradjust"
#define THREAD_NAME_UPNP			"upnp"

#define THREAD_NAME_DHCP6C			"dhcp6c"
#define THREAD_NAME_CREATE_RAID		"createRaid"
#define	THREAD_NAME_RETRIEVE_EXPORT "retrieveExp"
#define	THREAD_NAME_MSFS_BACKUP 	"msfs_backup"
#define	THREAD_NAME_MSFS_TIEMR 		"msfs_timer"
#define	THREAD_NAME_MSFS_SID 		"msfs_sid"
#define	THREAD_NAME_BACKUP_SNAPSHOT "backup_snapshot"
#define	THREAD_NAME_BACKUP_AVI		"backup_avi"

#define THREAD_NAME_MULTICAST		"multicast"
#define THREAD_NAME_MULTICAST_EX	"multicast_ex"
#define THREAD_NAME_DHCP6C			"dhcp6c"
#define THREAD_NAME_STATUS_UPDATE	"status_update"

#define THREAD_NAME_SMART_EVENT		"smartEvent"

#define THREAD_NAME_FAILOVER_SEARCH	"failover_search"
#define THREAD_NAME_FAILOVER_LIBUV	"failover_libuv"
#define THREAD_NAME_FAILOVER		"failover"



//default value (params)
#define DEF_MAX_CAMERA_NUM		32
#define DEF_MAX_ALARM_IN_NUM	MAX_ALARM_IN
#define DEF_MAX_ALARM_OUT_NUM 	MAX_ALARM_OUT
#define DEF_LANG				"65535"
#define DEF_HTMI_HOTPLUG		1

//network device
#define MAX_DEVICE_NUM 3
#define DEVICE_NAME_BOND "bond0"
#define DEVICE_NAME_ETH0 "eth0"
#define DEVICE_NAME_ETH1 "eth1"
#define DEVICE_NAME_PPPOE "ppp0"

#define SNAPSHOT_EXPORT_UTIL		"picexport"
#define	SNAPSHOT_QUALITY			50

#define POE_DEFAULT_PASSWORD		"ms1234"

//end bruce.milesight add
//////////////////////////////////////////////////////////////
#define HTTP_HEADER_VERSION (20170301)
#define HTTP_HEADER_BOUNDARY (0x00112233)

struct frame_packet_header
{
	int version;
	int ch;
	int strm_type;
	int codec_type;
	int stream_from;
	int stream_format;

	unsigned long long time_usec;
	unsigned int time_lower;
	unsigned int time_upper;
	int size;
	int frame_type;
	int frame_rate;
	int width;
	int height;
	//for audio
	int sample;
	int bps;
	int achn;
	//for expansion
	int expansion0;
	int expansion1;
};


typedef struct http_frame_packet_header_s
{
	int boundary;			  //0x00112233
	int version;			  //version
	char hsession[32];         //http session id 
	unsigned int header_size; // header size
	int ch;					  // ch number
	int strm_type;			  // strm_type: video data or audio data
	unsigned int size;        // size
	
	// for video 
	int codec_type; 		  // video codec type
	int frame_type; 		  // i-frame or p-frame 
	int width;
	int height;
	
	//for audio
	int audio_codec;         // -1 for no audio 
	int sample;
	
	unsigned long long time_usec;  	
}http_frame_packet_header_t;

struct UrlPara{
	char mac[MAX_LEN_32];
	char ip[MAX_LEN_32];
	UINT hPort;
	UINT rPort;
	DEVICE_TYPE type;
	char spwd[MAX_LEN_256];
	char softver[MAX_LEN_32];
};
struct squestion
{
	int enable;
	int sqtype;
	char squestion[MAX_LEN_64*3];
	char answer[MAX_LEN_64*3];
};

typedef enum ms_active_camera_type{
	ACTIVATE = 1,
	SETSQA,
}ACTIVE_CAMERA_TYPE;

struct active_camera
{
	char mac[MAX_LEN_32];
	char cChallengeKey[MAX_LEN_1024];
	int KeyStatus;
	long KeyID;
	ACTIVE_CAMERA_TYPE type;
};
struct active_camera_info
{
	int camera_list;
	ACTIVE_CAMERA_TYPE type;
	char mac[MAX_LEN_64][MAX_LEN_32];
	char password[MAX_LEN_32];	
	struct squestion sqas[3];
};
struct reset_type
{
	int keepIp;
	int keepUser;
};
struct check_sqa_params
{
	int allow_times;
	int lock_remain_time;
};
typedef struct ms_login_user_t	
{ 
	char login_user[MAX_LEN_128];
	int allow_times;
	long first_login_time;
	long last_login_time;
}ms_login_user_t;

////////////////////////MSFS
///////////////////////
#endif//_MS_DEFS_H_
