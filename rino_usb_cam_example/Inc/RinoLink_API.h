
#ifndef _RINOLINK_AIP_H_
#define _RINOLINK_AIP_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define SN_PID_LEN 			16
#define SN_UUID_LEN 		32
#define SN_SECRET_LEN 		128
#define SN_MAC_LEN 			16


#define CMD_TYPE_REG_IFRAME                  "req_key_frame"
#define CMD_TYPE_STARTTALK                  "Start_Talk"
#define CMD_TYPE_STOPTALK                   "Stop_Talk"
#define CMD_TYPE_BITRATE                    "bitrate_changed"
#define CMD_TYPE_STARTSTREAM                "Start_StreamPlay"
#define CMD_TYPE_ENDSTREAM                  "Sop_StreamPlay"
#define CMD_TYPE_ALEXA                      "Alexa_StreamPlay"
#define CMD_TYPE_STAORAGERROR               "Storage_Error"
#define CMD_TYPE_USER_JOIN_CHANNEL          "user_join_channel"
#define CMD_TYPE_USER_LEAVE_CHANNEL         "user_leave_channel"
#define CMD_TYPE_USER_MUTE_AUDIO            "user_mute_audio"
#define CMD_TYPE_USER_MUTE_VIDEO            "user_mute_video"

#pragma pack(1)
typedef struct {

	unsigned char 		pid[SN_PID_LEN];
	unsigned char 		uuid[SN_UUID_LEN];
	unsigned char  		secret[SN_SECRET_LEN];
	unsigned char  		mac[SN_MAC_LEN];
} Rinolink_SnParam_t;
#pragma pack()

#pragma pack(1)
typedef struct __Rino_LocalRecordInfo_t
{
	unsigned short           isaudiomute;        	//是否录制音频, 0:需要音频 1：禁用音频
	unsigned short           isencrypt;        	    //是否进行加密
	unsigned short           recordtpye;            //TF卡录像功能类型 1:事件录像，2：连续录像,0:不启动
	unsigned short           Channelcount;          //摄像头的数量
    char                     encryptkey[128];       //加密密钥
    char                     storagePath[64];       //存储设备的路径
}Rino_LocalRecordInfo_t;
#pragma pack()


typedef enum RINO_ALARM_TYPE
{
    RINO_TYPE_PIR = 1,
    RINO_TYPE_MOTION,         // 移动侦测事件
    RINO_TYPE_HUMAN,          // 人形侦测事件
    RINO_TYPE_FACE,           // 人脸测事件
    RINO_TYPE_VEHICLE,        // 车辆事件
    RINO_TYPE_PLATE,          // 车牌事件
    RINO_TYPE_VOICE,          // 声音告警事件

    RINO_TYPE_PASSBY,         // 有人经过
    RINO_TYPE_LINGER,         // 车辆事件
    RINO_TYPE_CAT,            // 车牌事件
    RINO_TYPE_BABY_CRY,       // 婴儿哭声
    RINO_TYPE_ANTIBREAK,      // 强拆报警
    RINO_TYPE_BANG,           // 异响
    RINO_TYPE_CALL,           // 呼叫
    RINO_TYPE_TEMP,           // 温度
    RINO_TYPE_FEEDING,        // 喂食
    RINO_TYPE_PACKAGE,        // 包裹
    RINO_TYPE_DOORBELL,       // 门铃
    RINO_TYPE_HOVER,          // 徘徊
    RINO_TYPE_FETCH_PACKAGE,  // 取包裹
    RINO_TYPE_HUM_AND_TEMP,   // 温湿度
    RINO_TYPE_FALL,           // 有人跌倒

    RINO_TYPE_CUSTOM1 = 0xF1, // 方案自定义事件1
    RINO_TYPE_CUSTOM2,        // 方案自定义事件2
    RINO_TYPE_CUSTOM3,        // 方案自定义事件3
    RINO_TYPE_CUSTOM4,        // 方案自定义事件4
    RINO_TYPE_CUSTOM5,        // 方案自定义事件5
} RINO_ALARM_TYPE;



//主事件类型,标识是那个模块的发出的事件
typedef enum RINO_EVENT_TYPE
{
    RINO_EVENT_TYPE_RECORD = 1,        //触发录像事件
    RINO_EVENT_TYPE_ALARM,             //告警事件触发具体事件查看ALARM_EVENT_TYPE
    RINO_EVENT_TYPE_HARDTRIGGER,       //外围硬件触发事件
    RINO_EVENT_TYPE_LEDCONTROL,        //灯控事件
    RINO_EVENT_TYPE_OTA,               //OTA升级事件
    RINO_EVENT_TYPE_STORAGE,           //存储空间
    RINO_EVENT_TPYE_STANDBY,           //设备休眠事件
    RINO_EVENT_TPYE_REBOOT,			  //重启事件，具体看REBOOT_EVENT_TYPE
    RINO_EVENT_TPYE_PARAMCHANGE,		  //参数改变
}RINO_EVENT_TYPE;

/**
 * \brief SDK环境变量结构体
 * \struct RinoSdkEnvVar_t
 */
#pragma pack(1)
typedef struct Rinolink_SdkEnvVar_t_ 
{
    unsigned int         devType; /**< 设备类型0表示ipc， 1表示bpi, 2表示nvr, 3表示基站 参考 IvyDevType_e 类型定义 */
    unsigned int         maxChannels; /**< 最大支持的通道数 */
	unsigned int         streanChanels;/**< 通道下支持的码流数量 1:单码流(实时流跟录像流公用一个流)，2:双码流（0：用于实时，1：用于录像） */
	unsigned int         maxdataspace;/*最小不低于512*1024*/  
    unsigned int         maxframesize;/*最大单帧数据大小,主要评估I帧的最大大小，SDK如果检测到送进了的帧大小大于设置的帧大小就直接丢弃*/  
    unsigned int         maxpalybackuser;/*最大容许同时回放录像的用户数,这个设置要评估设备的资源*/  
    unsigned int         maxcloudrecordtimes;/*单个最大云录像片段的大小单位秒,SDK内部默认最小是2秒一个片段，设置数值越大在无TF卡的情况下占用的内存越大，建议1080P分别率设置为4秒*/  
    unsigned int         maxviewuser;/*最大允许同时观看人数，直播+回放*/
    unsigned int         videoCodec;//视频编码格式,默认h264,请参考 @Rinolink_MeidaFormat_e
    unsigned int         audioCodec;//音频编码格式,默认g711u webrtc只支持g711a和g711u,请参考 @Rinolink_MeidaFormat_e
    unsigned int         audioSample;//音频采样率
    unsigned int         isCloudBindStatus;//以云端绑定状态为准
    char                 upload_timeout; /* 上传超时时间 3~60秒 */
    char                 cloudRetry;/*上传录像重试时间 */
    char                 configfilePath[SN_SECRET_LEN];/**< SDK内部配置文件存放路径，要求可读写，掉电不丢失 */
	char                 storagePath[SN_SECRET_LEN];/**< 存储设备路径，用于本地录像存储，如果为空表示不支持本地存储设备 */
	char                 mqttserverurl[SN_SECRET_LEN];/**< mqtt 服务器地址 */
    char                 country[SN_SECRET_LEN];/**< 国家码 */
    char                 *cacertData;/**< 证书内容 */
	char                 *deviceparamterbuff;/*设备保存本地的配置参数，初始化sdk的时候需要上传给后台同步给客户端,格式如下：*/
											/* {"properties":{
														"color":"red",  //颜色  红色
														"brightness":80 //亮度  80
											                 },
                                                "sw_version":"1.0.1"}*/
    unsigned int         isMultiFrameRate;/* 是否有多帧率 */
    unsigned int         deviceparamterlen;/*设备保存本地的配置参数长度*/
    //char                 VideoResolution[2][16];/*标识每个流通道的视频能力，用于分辨率的切换*/
    unsigned int         encryptionRules;/*加密规则，默认0,1是开启SSL,2是开启数据加密*/
    
    unsigned char        quickstartinfo[10 * 1024];//快起时候推流信息
    unsigned int         domain_limit;//是否开启狱限制。        
	char                 unuse[64];//预留
    char                 factoryPath[SN_SECRET_LEN];/**< 产测设备路径，用于产测产生的一些文件 */
}Rinolink_SdkEnvVar_t;
#pragma pack()

#pragma pack(1)
typedef struct __Rinolink_EventInfo_t
{
	unsigned long long   event_pts;
    unsigned int         validtime;  //报警有效时间10~60秒,决定事件录像的长度
	unsigned int         mainevent_type; //主事件EVENT_TYPE
    unsigned int         subevent_type;  //子事件LED_CONTROL_EVENT，HARDTRIGGER_TYPE,STORAGE_STATUS，OTA_STATUS
    unsigned int         app_push_type; //如果为1则云端不推送
    unsigned int         is_offline;   //是否离线录像      
	char                 event_data[256];
    char                 detection_area[32]; // 侦测区域
}Rinolink_EventInfo_t;
#pragma pack()

typedef enum {

	RINO_VIDEO_PFRAME = 0,									/**< 0 */
	RINO_VIDEO_IFRAME = 1,
	RINO_AUDIO_FRAME = 2,
}Rinolink_FrameType_e;

typedef enum {

	RINO_VIDEO_H264 = 0,										/**< 0 */
    RINO_VIDEO_H265 = 1,
	RINO_VIDEO_YUV  = 2,
	RINO_VIDEO_JPEG = 3,

	RINO_AUDIO_PCM = 4,
	RINO_AUDIO_AAC = 5,
    RINO_AUDIO_G711A = 6,
    RINO_AUDIO_G711U = 7,
}Rinolink_MeidaFormat_e;

#pragma pack(1)
typedef struct {

	unsigned int        		u32Width;      	//分辨率宽度
    unsigned int        		u32Height;      //分辨率高度
    unsigned int        		u32Framerate;     //码率大小
    Rinolink_MeidaFormat_e     	eFormat;  		//编码格式
}Rinolink_VideoInfo_t;
#pragma pack()

#pragma pack(1)
typedef struct {
	
    unsigned int 				u32Samplerate;  //采样频率
    Rinolink_MeidaFormat_e     	eFormat;  		//编码格式
} Rinolink_AudioInfo_t;
#pragma pack()

#pragma pack(1)
typedef struct {
	
	char *  			        pDataBuf;		//数据
	unsigned int     			u32BufSize;     //数据大小
	Rinolink_FrameType_e     	eType;  		//帧类型
	unsigned int        		u32Seq;   		//帧序号	
	unsigned long long  		u64Timestamp;  	//时间戳
 
    union 
    {
       Rinolink_VideoInfo_t 	stVideoInfo;
       Rinolink_AudioInfo_t 	stAudioInfo;
    } Media_Packet_Info;	
	unsigned int                nchannel;       //对讲通道号,用于处理多路设备的对讲，例如nvr设备
} Rinolink_MediaPacket_t;
#pragma pack()

#pragma pack(1)
typedef struct {
	int resCode;
    int firmwareType;  //工厂固件为1，标准固件为2，mcu固件为3 
    int percent;  //下载进度 0~100
    char *version; //设备当前版本号
    char *targetVersion;  //升级的目标版本号
} Rinolink_Ota_Progress_t;
#pragma pack()


/**
 * 当SDK初始化或者网络重连时，一定会触发RINOLINK_UNBIND和RINOLINK_DEVICEONLINE回调
*/
typedef enum 
{
    RINOLINK_UNBIND ,//初始化SDK或者网络重连时,设备没有被绑定,会回调此消息
	RINOLINK_BINDED,//绑定设备成功以后回调此消息,事件消息
    RINOLINK_DEVICEONLINE,//当初始化或者网络重连时,云端和本地都在绑定状态，回调此消息,如果云端已经把设备删除,初始化时会回调RINOLINK_UNBIND
	RINOLINK_RESET,// 解绑设备，清除设备配置，清除配网信息
    RINOLINK_CLEAN_NETWORK, // 不解绑设备，不清除设备配置，清除配网信息
    RINOLINK_CLEAN_DATA, // 不解绑设备，清除设备配置，不清除配网信息
	RINOLINK_OTA,//告诉设备需要升级,事件消息
    RINOLINK_FACTORY,//设备进入产测模式,事件消息
    RINOLINK_CLOUD_CONNECT, //连接上云端服务，设备完全就绪请订阅 RINOLINK_DEVICEONLINE
    RINOLINK_CLOUD_DISCONNECT //与云端断开连接
}Rinolink_Status_e;

typedef enum RINO_LOG_LEVEL
{
    RINOLINK_LOG_DEBUG = 1,
    RINOLINK_LOG_INFO,    
    RINOLINK_LOG_WARN, 
    RINOLINK_LOG_ERROR, 
}RINO_LOG_LEVEL;


#define    OPRT_FAIL        -1   //非0代表失败
#define    OPRT_OK           0   //成功



// 定义状态回调函数类型
typedef void (*Rinolink_StatusCallback)(const Rinolink_Status_e Rinolink_status,char *data,int datalen);

//定义控制信令接受回调
typedef void (*Rinolink_CmdDataCallback)(const char *cmdjData,char *response,int *responselen);

//定义控制对讲音频数据回调
typedef void (*Rinolink_PushTalkDataCallback)(Rinolink_MediaPacket_t *talkdata);


//定义控制对讲视频数据回调
typedef void (*Rinolink_PushVideoDataCallback)(Rinolink_MediaPacket_t *videodata);



typedef struct {

    /*****************************************************************************
     函 数 名  : Rinolink_QueryRecordByMonth
    功能描述  :  通过具体的月时间查询月下面那天有录像
    输入参数  :  channel     控制信令的回调接口
                 month       具体的月时间"202403"

    输出参数  :  result       返回的查询结果 "3,5,6,16.22,25"
    返 回 值: 成功返回OPRT_OK
    *****************************************************************************/
    void (*Rinolink_QueryRecordByMonth)(int channel,char *month,char *result);
    
    /*****************************************************************************
    函 数 名  : Rinolink_QueryRecordByday
    功能描述  :  通过具体的日期时间查询当天下面录像时间片段
    输入参数  :  channel     镜头通道号
                month       具体的日期时间"20240318"
        
    输出参数  :  result      "1710739896~1710740076~0|1710740076~1710740256~0|1710740256~1710740436~0|1710740436~1710740617~0"
                                 ^          ^        ^
                                 |          |        |
                              开始时间   结束时间    片段分割符

    返 回 值: 成功返回OPRT_OK
    *****************************************************************************/
    void (*Rinolink_QueryRecordByday)(int channel,char *day,char *result);
    
    /*****************************************************************************
    函 数 名  : Rinolink_DeleteRecordByTimes
    功能描述  : 通过时间片段信息删除具体的录像文件
    输入参数  : cmdcallback     控制信令的回调接口
        
    输出参数  : 无
    返 回 值: 成功返回OPRT_OK
    *****************************************************************************/
    void (*Rinolink_DeleteRecordByTimes)(int channel,char *deleteinfo);


    /*****************************************************************************
    函 数 名  : Rinolink_PlayBackStart
    功能描述  : 根据传入的时间节点定位到具体文件进行播放
    输入参数  : channel     镜头通道号
               playtime    跳转时间具体到秒 1710921107
        
    输出参数  : playhandle  返回播放句柄
    返 回 值: 成功返回OPRT_OK
    *****************************************************************************/
    void *(*Rinolink_PlayBackStart)(int channel,int playtime);

    /*****************************************************************************
    函 数 名  : Rinolink_PlayBackStop
    功能描述  : 停止播放
    输入参数  : channel     镜头通道号
               playhandle  回放文件读取句柄
        
    输出参数  : 无
    返 回 值: 成功返回OPRT_OK
    *****************************************************************************/
    void (*Rinolink_PlayBackStop)(int channel,void *playhandle);

    /*****************************************************************************
    函 数 名  : Rinolink_PlayBacSeek
    功能描述  : 录像播放时间跳转
    输入参数  : channel     镜头通道号
               playhandle  回放文件读取句柄
               playtime    跳转时间具体到秒 1710921107  
        
    输出参数  : 无
    返 回 值: 成功返回OPRT_OK
    *****************************************************************************/
    void *(*Rinolink_PlayBacSeek)(int channel,void *playhandle,int playtime);

    /*****************************************************************************
    函 数 名  : Rinolink_PlayBacGetNextOneFrame
    功能描述  : 依据传入的playhandle读取回放文件的下一帧数据，
    输入参数  : channel     镜头通道号
               playhandle  回放文件读取句柄 
        
    输出参数  : *frame      返回一帧的音视频数据。frame->pDataBuf需要进行存储空间申请
    返 回 值: 成功返回OPRT_OK
    *****************************************************************************/
    int (*Rinolink_PlayBacGetNextOneFrame)(int channel,void *playhandle,Rinolink_MediaPacket_t *frame);

}Rinolink_RecordPlayBack_Handler_t;

/*****************************************************************************
 函 数 名  : Rinolink_RegisterRecordPlayBackCallback
 功能描述  : 注册回放录像文件需要相关接口
 输入参数  : cmdcallback     控制信令的回调接口
 	
 输出参数  : 无
 返 回 值: 成功返回OPRT_OK
*****************************************************************************/
int Rinolink_RegisterRecordPlayBackCallback(Rinolink_RecordPlayBack_Handler_t  *recordplaybackstatecallback);



/*****************************************************************************
 函 数 名  : Rinolink_RegisterStatusCallback
 功能描述  : SDK相关的状态回调具体内容可以参考Rinolink_status_t
 输入参数  : cmdcallback     控制信令的回调接口
 	
 输出参数  : 无
 返 回 值: 成功返回OPRT_OK
*****************************************************************************/
int Rinolink_RegisterStatusCallback(Rinolink_StatusCallback  statecallback);

/*****************************************************************************
 函 数 名  : Rinolink_RegisterCmdCallback
 功能描述  : APP或MQTT发送过来的控制信息都会通过这个回调给到设备端进行相关动作
 输入参数  : cmdcallback     控制信令的回调接口
 	
 输出参数  : 无
 返 回 值: 成功返回OPRT_OK
*****************************************************************************/
int Rinolink_RegisterCmdCallback(Rinolink_CmdDataCallback  cmddatacallback);

/*****************************************************************************
 函 数 名  : Rinolink_AVPushData
 功能描述  : 设备向SDK推动音视频内容
 输入参数  : channel       通道号,如果是摄像头产品，则通道号为0，如果是nvr等产品，channel则为前端摄像头的索引
 		    streamtype    准备传送的内容，0:主码流，1:从码流，2:第三码流。
            pstMediaPacket     Rinolink_MediaPacket_t 
 输出参数  : 无
 返 回 值: 成功返回OPRT_OK
*****************************************************************************/
int Rinolink_AVPushData(int channel,int streamtype,Rinolink_MediaPacket_t *pstMediaPacket);

/*****************************************************************************
 函 数 名  : Rinolink_RegisterPushTalkDataCallback
 功能描述  : 注册一个接受音频对讲的数据回调函数
 输入参数  : cmdcallback     控制信令的回调接口
 	
 输出参数  : 无
 返 回 值: 成功返回OPRT_OK
*****************************************************************************/
int Rinolink_RegisterPushTalkDataCallback(Rinolink_PushTalkDataCallback  talkdatacallback);


/*****************************************************************************
 函 数 名  : Rinolink_RegisterPushVideoDataCallback
 功能描述  : 注册一个接受视频对讲的数据回调函数
 输入参数  : cmdcallback     控制信令的回调接口
 	
 输出参数  : 无
 返 回 值: 成功返回OPRT_OK
*****************************************************************************/
int Rinolink_RegisterPushVideoDataCallback(Rinolink_PushVideoDataCallback  videodatacallback);


/*****************************************************************************
 函 数 名  : Rinolink_DownloadOtaFile
 功能描述  : 注册一个接收ota文件的回调函数
 输入参数  : 
 	        request_uri         文件的下载url
			filepath            下载文件的存储路径
 输出参数  : 无
 返 回 值: 成功返回OPRT_OK
*****************************************************************************/
int Rinolink_DownloadOtaFile(char *request_uri,char *filepath);



/*****************************************************************************
 函 数 名  : Rinolink_UploadParamer
 功能描述  : 通过这个接口上传物模型产生
 输入参数  : 
 	        uploaddata        上传参数json格式字符串
            {"properties":{
                            "color":"red",  //颜色  红色
                            "brightness":80 //亮度  80
			              }
            }

			uploaddatalen      上传参数长度
 输出参数  : 无
 返 回 值: 成功返回OPRT_OK
*****************************************************************************/
int Rinolink_UploadParamer(char *uploaddata,int uploaddatalen);


/*****************************************************************************
 函 数 名  : Rinolink_UploadNetworkStatus
 功能描述  : 上传网络状态
 输入参数  : 
 	        uploaddata        上传参数json格式字符串
            {
                "networkType":1,//网络类型：1:wifi,2:有线
                //信号强度：1：好；2:中；3：差 (好：Ping 响应时间通常在10毫秒（ms）到50毫秒（ms）范围内, 中：Ping 响应时间通常在50毫秒（ms）到100毫秒（ms）范围内。差：Ping 响应时间超过100毫秒（ms），通常在数百毫秒范围内)
                "signal":1,
                "signalValue":100 //0-100  0%-100%
            }
			uploaddatalen      上传参数长度
 输出参数  : 无
 返 回 值: 成功返回OPRT_OK
*****************************************************************************/
int Rinolink_UploadNetworkStatus(char *uploaddata,int uploaddatalen);

/*****************************************************************************
 函 数 名  : Rinolink_Upload_Ota_Progress
 功能描述  : 上传ota进度
 输入参数  : 
 输出参数  : 无
 返 回 值: 成功返回OPRT_OK
*****************************************************************************/
int Rinolink_Upload_Ota_Progress(Rinolink_Ota_Progress_t *ota_progress);


/*****************************************************************************
 函 数 名  : Rinolink_UploadParamer
 功能描述  : 通过这个接口上传物模型产生
 输入参数  : 
 	        //测试步骤：1：设备自检；2：TF卡测试；3：复位键测试；
            //4：4G信号测试；5：wifi信号测试；6：有线网测试
            "setup":1,
            "result":"success",//测试结果：success 测试成功；fail：测试失败
            "msg":"测试成功"//结果描述 
 输出参数  : 无
 返 回 值: 成功返回OPRT_OK
*****************************************************************************/
int Rinolink_UploadFactoryTestResult(int setup,char *result,char *msg);


/*****************************************************************************
 函 数 名  : Rinolink_UploadFactoryTestSettings
 功能描述  : 通过这个接口上传产测参数
 输入参数  : 
 	   {
        "version":"1.0.0",//主控版本
        "wifiVersion":"0.0.0",//wifi版本  wifi类型设备时回复
        "cardNo":"5678956789",//4g卡号  4g类型设备时回复
        "authStatus":0,//授权状态 0：未授权；1：已授权
        "signal4G":66, //4G信号强度  4g类型设备时回复
        "signalWifis":[ //wifi信号强度，wifi类型设备时回复
            {
                "signal":100,//信号强度
                "name":"kddev"//wifi名称
            },
            {
                "signal":95,//信号强度
                "name":"kddev2"//wifi名称
            },
        ],
        "uuid":"xxx"   //授权uuid  已授权时回复
        "secret":"授权密钥",  //授权密钥  已授权时回复
        "mac":"授权mac地址",  //授权mac地址  已授权时回复        
    }  
 输出参数  : 无
 返 回 值: 成功返回OPRT_OK
*****************************************************************************/
int Rinolink_UploadFactoryTestSettings(char *uploaddata,int uploaddatalen);


/*****************************************************************************
 函 数 名  : rinolink_event_pushdata
 功能描述  : 设备向SDK推动报警事件内容
 输入参数  : channel       通道号,如果是摄像头产品，则通道号为0，如果是nvr等产品，channel则为前端摄像头的索引
            event_info    事件的内容具体请参考Rinolink_Event_Info_t结构体。 
 输出参数  : 无
 返 回 值: 成功返回OPRT_OK
*****************************************************************************/
int Rinolink_EventPushData(int channel,Rinolink_EventInfo_t *eventinfo);

/*****************************************************************************
 函 数 名  : Rinolink_BindDevice
 功能描述  : 当SDK返回没有绑定的状态需要调用这个接口进行设备绑定
 输入参数  : token  绑定时app提供的token
           
 输出参数  : 无
 返 回 值: 成功返回OPRT_OK
*****************************************************************************/
int Rinolink_BindDevice(const char *token);

/*****************************************************************************
 函 数 名  : Rinolink_BindDevice
 功能描述  : 当SDK返回没有绑定的状态需要调用这个接口进行设备绑定
 输入参数  : userId  用户id
 输入参数  : assertId  资产id
 输入参数  : mcuVersion  mcu版本, 格式 "x.y.z"
           
 输出参数  : 无
 返 回 值: 成功返回OPRT_OK
*****************************************************************************/
int Rinolink_BindDevice_By_User(const char *userId, const char* assertId, const char* version, const char* mcuVersion);


/*****************************************************************************
 函 数 名  : Rinolink_UnBindDevice
 功能描述  : 方案商可以调用这个接口进行sdk和后台服务器绑定
 输入参数  : iscleandata 是否清理后台数据 0:不清理数据，1：清理数据
           
 输出参数  : 无
 返 回 值: 成功返回OPRT_OK
*****************************************************************************/
int Rinolink_UnBindDevice(int iscleandata);


/*****************************************************************************
 函 数 名  : Rinolink_GetServerTime
 功能描述  : 获取当前服务器的时间
 输入参数  : times  返回的时间格式
            timeslen 返回时间字符串的长度
 输出参数  : 无
 返 回 值: 成功返回OPRT_OK
*****************************************************************************/
int Rinolink_GetServerTime(char *times,int *timeslen);


/*****************************************************************************
 函 数 名  : Rinolink_GetSdkRunStatus
 功能描述  : 获取当前SDK的运行状态
 输入参数  : *status  返回的sdk的状态
            RINO_SDKRUN_IDLE = 0,  //空闲无流媒体传输，无录像存储，无报警上报
            RINO_SDKRUN_BUSY,     //处于流媒体传输|无录像存储|无报警上报

 输出参数  : 无
 返 回 值: 成功返回OPRT_OK
*****************************************************************************/
int Rinolink_GetSdkRunStatus(int *status);


/*****************************************************************************
 函 数 名  : Rinolink_StopLocalRecord
 功能描述  : 停止本地录像服务,进行格式化操作时必须调用改接口停止录像服务
 输入参数  : 无
           
 输出参数  : 无
 返 回 值: 成功返回OPRT_OK
*****************************************************************************/
int  Rinolink_StopLocalRecord();

/*****************************************************************************
 函 数 名  : Rinolink_StartLocalRecord
 功能描述  : 开启本地录像服务
 输入参数  : localrecorinfo  录像参数配置,具体参数请看Rino_LocalRecordInfo_t结构体

 输出参数  : 无
 返 回 值: 成功返回OPRT_OK
*****************************************************************************/
int Rinolink_StartLocalRecord(Rino_LocalRecordInfo_t *localrecorinfo);


/*****************************************************************************
 函 数 名  : Rinolink_Initialize
 功能描述  : 初始化整个Rino云sdk
 输入参数  : pstSNParam    注册三元组
           
 输出参数  : 无
 返 回 值: 成功返回OPRT_OK
*****************************************************************************/
int Rinolink_Initialize(Rinolink_SnParam_t *pstSNParam,Rinolink_SdkEnvVar_t *pstSdkEnvVar);

/*****************************************************************************
 函 数 名  : Rinolink_Initialize
 功能描述  : 反初始化整个Rino云sdk
 输入参数  : 无
           
 输出参数  : 无
 返 回 值: 成功返回OPRT_OK
*****************************************************************************/
int Rinolink_DeInitialize(void);


/*****************************************************************************
 函 数 名  : Rinolink_log_file_upload
 功能描述  :  上传日志文件
 输入参数  :  file_path  日志文件路径
           
 输出参数  : 无
 返 回 值: 成功返回OPRT_OK
*****************************************************************************/
int Rinolink_log_file_upload(char *file_path);

/*****************************************************************************
 函 数 名  : Rinolink_SetLogLevel
 功能描述  :  设置日志级别
           
 输出参数  : 无
 返 回 值: 无
*****************************************************************************/
void Rinolink_SetLogLevel(RINO_LOG_LEVEL level);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
