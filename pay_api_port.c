#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <jni.h>
#include <android/Log.h>
#include <stdio.h>
#include <string.h>
#include <linux/threads.h>
#include <pthread.h>
#include <strings.h>

#define	CMD_TERMCMD		0xA0//设备指令
#define	CMD_InPassword		0xA1//进入升级模式
#define	CMD_RFID			0xA2//非接卡
#define	CMD_ICCARD			0xA3//ic卡
#define	CMD_PSAM1			0xA4//psam卡1
#define	CMD_PSAM2			0xA5//psam卡2
#define	CMD_BRUSH			0xA6 //刷卡(保留)
#define	CMD_M1				0xA7//mifire one卡

#define CMD_COUNT           0x01 //总项目数



static int nMaxDatalen  =1024;
static int mTtyfd = -1;
static int mOpen = 0;
static  jboolean D =1; //是否打开LOG
static  int  CMD_PACK_HEAD = 0x02;
static  int  CMD_PACK_END = 0x03;
static  int ReadTimeout = 20000;
static int ReadTimeDely = 1; //读取数据延时时间判断标志
jboolean   OperWrite =JNI_FALSE; //根据情况判断是否在操作应用功能
static  int pasmcard =1; //如果不设置,默认PASM1卡

//读取需要用到的函数

long nTimeout =600000;
int Readbytes =0;
int ReadTotalbytes =0;
jbyte  Readbuffer[1024];
 int nDatalen = 0;
jboolean CheckAck =JNI_FALSE; //是否读取校验码
jboolean  CheckData  =JNI_FALSE ; //是否独到数据 JNI_TRUE





static const char *TAG="com_pay_uart_port";
#define LOGI(fmt, args...) __android_log_print(ANDROID_LOG_INFO,  TAG, fmt, ##args)
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, TAG, fmt, ##args)
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, TAG, fmt, ##args)

int speed_arr[]={B38400, B19200, B9600, B4800, B2400, B1200, B300,
    B38400, B19200, B9600, B4800, B2400, B1200, B300,};
int name_arr[]={38400, 19200, 9600, 4800, 2400, 1200, 300, 38400,
    19200, 9600, 4800, 2400, 1200, 300,};



void* threadreadTtyData(void *args);
static JNIEnv *jenv =NULL;
static jmethodID midMain =0;
static jclass classWA;






static speed_t getBaudrate(jint baudrate)
{
	switch(baudrate) {
	case 0: return B0;
	case 50: return B50;
	case 75: return B75;
	case 110: return B110;
	case 134: return B134;
	case 150: return B150;
	case 200: return B200;
	case 300: return B300;
	case 600: return B600;
	case 1200: return B1200;
	case 1800: return B1800;
	case 2400: return B2400;
	case 4800: return B4800;
	case 9600: return B9600;
	case 19200: return B19200;
	case 38400: return B38400;
	case 57600: return B57600;
	case 115200: return B115200;
	case 230400: return B230400;
	case 460800: return B460800;
	case 500000: return B500000;
	case 576000: return B576000;
	case 921600: return B921600;
	case 1000000: return B1000000;
	case 1152000: return B1152000;
	case 1500000: return B1500000;
	case 2000000: return B2000000;
	case 2500000: return B2500000;
	case 3000000: return B3000000;
	case 3500000: return B3500000;
	case 4000000: return B4000000;
	default: return -1;
	}
}
/*
 *
 *  回调类
 *
 *
 */

/**
 * class Listener
 */

//当动态库被加载时这个函数被系统调用

 jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
	 JNIEnv* env = NULL;
     jint result = -1;
     //获取JNI版本
     LOGI("Radio JNI_OnLoad");
     if ((*vm)->GetEnv(vm, (void**)&env, JNI_VERSION_1_4) != JNI_OK){
         LOGE("ERROR: GetEnv failed/n");
         goto fail;
     }
    if(env == NULL){
         goto fail;
     }

     /* success -- return valid version number */
     result = JNI_VERSION_1_4;
 fail:
     return result;
}

 void CallBackData(jbyte data[],int nlength){


	 jbyteArray CallOutData=NULL;
	 if ((midMain ==0) || (jenv ==NULL) || (nlength ==0)) return;
	 if (D) LOGE("jni callback (1) nlen:%d",nlength);
    // if (CallOutData ==NULL)
      CallOutData = (*jenv)->NewByteArray(jenv,nlength);
     if (D) LOGE("jni callback (2)");
     if (CallOutData ==NULL)
     {
     	 if (D) LOGE("jni CallOutData==NULL");
    	 return;
     }
     (*jenv)->SetByteArrayRegion(jenv,CallOutData,0,nlength,data);
     if (D) LOGE("jni callback (3)");
     (*jenv)->CallStaticVoidMethod(jenv,classWA,midMain,CallOutData,nlength);  //调用此静态方法
     if (D) LOGE("jni callback Sucess(4)");
    // (*jenv)->ReleaseByteArrayElements(jenv,CallOutData,data,0);

    // (*jenv)->DeleteLocalRef(CallOutData);
     CallOutData=NULL;
 }
 //static const char* classPathName="android/serialport/SerialPort";

 char* jstringTostring(JNIEnv* env, jstring jstr)
 {
        char* rtn = NULL;
        jclass clsstring = (*env)->FindClass(env,"java/lang/String");
        jstring strencode = (*env)->NewStringUTF(env,"utf-8");
        jmethodID mid = (*env)->GetMethodID(env,clsstring, "getBytes", "(Ljava/lang/String;)[B");
        jbyteArray barr= (jbyteArray)(*env)->CallObjectMethod(env,jstr, mid, strencode);
        jsize alen = (*env)->GetArrayLength(env,barr);
        jbyte* ba = (*env)->GetByteArrayElements(env,barr, JNI_FALSE);
        if (alen > 0)
        {
                  rtn = (char*)malloc(alen + 1);
                  memcpy(rtn, ba, alen);
                  rtn[alen] = 0;
        }
        (*env)->ReleaseByteArrayElements(env,barr, ba, 0);
        return rtn;
 }

 static const char* classPathName="android/serialport/sample/MainMenu";
 int  Java_android_serialport_SerialPort_setCallBack
 (JNIEnv *env,jobject obj,jstring strclassname,jstring strcallname)
 {

	   jenv =env;

	   char*  className =jstringTostring(env,strclassname);
	   char*  callname =jstringTostring(env,strcallname);

       if (D) LOGE("jni callback (0) class =%s,callname =%s",className,callname);
       classWA = (*env)->FindClass(env, className);   //classWA就指向要调用的类(大家都知道java中都是以类组织的，所以想调用函数，首先要找到它所在的类)
       if (classWA ==NULL) return -1;

      // midMain = (*env)->GetStaticMethodID(env, classWA,"Callback","(Ljava/lang/String;)V");  //找到相应的函数,最后一个参数是签名，用javap -s -p 命令拿到的,待会细说。)
      // midMain = (*env)->GetStaticMethodID(env, classWA,"Callback"," (Ljava/lang/String;java/lang/I)V");
       midMain = (*env)->GetStaticMethodID(env, classWA,callname,"([BI)V");
       if (midMain ==0) return -2;
       if (D)LOGE("fields.post_event=%d",midMain);


/*
      jstring StringArg = (*env)->NewStringUTF(env,(char*)"http123456778");  //将char*的字符串转成java的String
      LOGE("jni callback (2)");
      (*env)->CallStaticVoidMethod(env,classWA,midMain,StringArg);  //调用此静态方法
       LOGE("jni callback (3)");
       */
       return 1;
 }












//标准API函数,发送函数

int Java_android_serialport_SerialPort_Checksum(jbyte data[],int nLen)
{
	int bcc = 0;
	int i;
	for(i=1;i<nLen +1;i++)
	{
		bcc = bcc^data[i];
	}
	return bcc;
}


JNIEXPORT jboolean JNICALL Java_android_serialport_SerialPort_write
  (JNIEnv *env, jobject obj, jint fd, jstring buff, jint len)
{
	jint ret;
    const char *buff_utf = (*env)->GetStringUTFChars(env, buff, JNI_FALSE);
    ret =write(fd, buff_utf, len);
    if(ret>0)
        return JNI_TRUE;
    else
       return JNI_FALSE;
}



/***
 * 校验ACK 没发送一包数据,必须校验ACK,若ACK校验正常,则可读取数据
 *
 * @return  boolean
 *      true 校验成功 false 校验
 * @throws IOException
 */








JNIEXPORT int JNICALL Java_Java_android_serialport_SerialPort_ReadComData
(jbyte bufferRev[])
{


    char Revicebuf[nMaxDatalen];
    int result = 0,ret,ntimeout,timepos=0;
    fd_set readfd;
    struct timeval timeout;
	ntimeout = ReadTimeout /ReadTimeDely *80;
    timepos =0;

    while(mOpen){

       timepos ++;
       if (timepos >ntimeout)
              break;

       timeout.tv_sec = 0;//设定超时秒数
       timeout.tv_usec = 80;//设定超时毫秒数
       memset(Revicebuf,0x00,nMaxDatalen);
       FD_ZERO(&readfd);//清空集合
       FD_SET(mTtyfd,&readfd);///* 把要检测的句柄mTtyfd加入到集合里 */

       ret = select(mTtyfd+1,&readfd,NULL,NULL,&timeout);/* 检测我们上面设置到集合readfd里的句柄是否有可读信息 */
       //if(D)LOGE("thread select ret: %d", ret);
       switch(ret){
       case -1:/* 这说明select函数出错 */
           result = -1;
           if(D)LOGE ("mTtyfd read failure");
           break;
       case 0:/* 说明在我们设定的时间值5秒加0毫秒的时间内，mTty的状态没有发生变化 */
           break;
       default:/* 说明等待时间还未到5秒加0毫秒，mTty的状态发生了变化 */
              if(FD_ISSET(mTtyfd,&readfd)){/* 先判断一下mTty这外被监视的句柄是否真的变成可读的了 */
              int bytes = read(mTtyfd,Revicebuf,sizeof(Revicebuf));
              /**发送数据**/
               if(D)LOGE("ReviceDATA: %d", bytes);
       	       if(D)
         	    {
       	    	  int nread=0;
         	      for (nread =0;nread <bytes;nread++)
         	      {
         	    	 // str =str +spintf("%x",Revicebuf[nread] &0xff);
         	    	  if(D)LOGE("read data i =%d,%x",nread,Revicebuf[nread] &0xff);
         	      }

         	    }
       	       if (bytes==1)
       	      {
       	            continue;
       	        }
       	       if (bytes >0)
       	       {
       	    	  if (D) LOGE("READ  read ReadTotalbytes:%d", ReadTotalbytes);
       	    	  memcpy(Readbuffer+ReadTotalbytes,Revicebuf,bytes);
       	    	  ReadTotalbytes =ReadTotalbytes+bytes;
       	    	  if ((Readbuffer[0]&0xff) ==0x02)
       	          {

       	                    		  nDatalen = Readbuffer[1] & 0xFF;
       	                    		  nDatalen <<= 8;
       	                    		  nDatalen |= Readbuffer[2] & 0xFF;	  //需要接收的总的大小
       	                    		   if (D) LOGE("READ nlen :", nDatalen);
       	                    		   if (nDatalen>0 && nDatalen <512)
       	                    			  Readbytes =nDatalen +5;
       	         }

       	    	 if (D) LOGE("READ  read ReadTotalbytes:%d,%d", ReadTotalbytes,Readbytes);
       	    	 if ((ReadTotalbytes >=Readbytes-1) && ReadTotalbytes >5){

       		          if (ReadTotalbytes +1== Readbytes)
       			        {
       			        	    	  Readbuffer[Readbytes-1] =0x03;
       			       }
       			        ReadTotalbytes =0;
       			        if (!CheckAck)
       			        {
       			        	     if (D) LOGE("READ buffer CheckAck true");
       			        	     CheckAck =JNI_TRUE;
       			        	     CheckData =JNI_FALSE;
       			        	  return Readbytes;
       			        }

       			        if (!CheckData) {
       			            CheckData=JNI_TRUE;
       			            if (D) LOGE("READ buffer CheckData true");
       			         return Readbytes;
       			      }
       	    	 }

       	       }
           }
           continue;
       }
       if(result == -1){
           break;
       }
    }
    if(D)LOGE ("stop run!");
    return Readbytes;


}





JNIEXPORT jboolean JNICALL Java_Java_android_serialport_SerialPort_ReadCheckUsbData
(jbyte bufferRev[])
{
	 jbyte buffer[nMaxDatalen];
	 jint size=0,timeout,timepos=0;
	 timeout = ReadTimeout /ReadTimeDely;
	 timepos =0;
	 if (D)  LOGE("READ  CheckACK Begin");
	 /*
	 while (JNI_TRUE){
		 	 timepos  ++;
			 if (timepos >timeout)
				 break;
			 if (CheckAck) //检测到ACKTRUE 直接退出
			 {
				 if (D)  LOGE("READ  CheckAck==JNI_TRUE ");
			 	 break;
			 }
		   if (D)  LOGE("READ  CheckAck %d",timepos);
			sleep(ReadTimeDely *10);
	 }
	 */
	 size =Java_Java_android_serialport_SerialPort_ReadComData(bufferRev);



	 if (D)  LOGE("READ  CheckACK END");
	 size =Readbytes;
	 memcpy(buffer,Readbuffer,size);
    if (D)  LOGE("READ  Checkdack atasize =%d", size);
	if (D)  LOGE("READ  CheckAck data:%d,%d,%d,%d",buffer[0] & 0xFF,buffer[14] & 0xFF,buffer[15] & 0xFF,buffer[17] & 0xFF);
    if (size  <= 0) return 0;
	  if (size  <= 0) return JNI_FALSE;
		if (buffer[0] != 0x02 || (buffer[14] & 0xFF) != 0xA0
				|| buffer[15] != 0x34 || buffer[16] != 0x00
				|| buffer[17] != 0x00) {
			 if (D) LOGE("check ack false");
			if (buffer[16] != 0xA0
					|| buffer[17] != 0x06){
				 memcpy(bufferRev,buffer,size);
		    	 return JNI_TRUE;
	     	}
			return JNI_FALSE;
		}
	 if (D) LOGE("check ack true");
	 return JNI_TRUE;
}



	/***
	 * 读取数据,循环读取
	 *

	 *@param OutbufferRev
	 *        传出的数据
	 *
	 *@param  dWaitTimeouts
	 *         读取的超时时间
	 * @return  int
	 *         读取数据大小
*/

JNIEXPORT jint JNICALL Java_Java_android_serialport_SerialPort_ReadCommData
(jbyte OutbufferRev[],jint dWaitTimeouts)
{
     jint size=0;
     jbyte bufferRev[nMaxDatalen];
     bzero(bufferRev, sizeof(bufferRev));
     CheckAck =JNI_FALSE;
     CheckData =JNI_TRUE;
 	if (D)  LOGE("ReadCommData while ACK being");

	 if (!Java_Java_android_serialport_SerialPort_ReadCheckUsbData(bufferRev))
	 		return 0;

	 if (bufferRev[16] == 0xA0
	 		|| bufferRev[17] == 0x06){
	 			size = bufferRev[1] & 0xFF;
	 			size <<= 8;
	 			size |= bufferRev[2] & 0xFF;
	 			return size + 5;
	 }

	 if (D)  LOGE("ReadCommData while ACK END");
     bzero(bufferRev, sizeof(bufferRev));
     bzero(Readbuffer, sizeof(Readbuffer));
     Readbytes =0;
     ReadTotalbytes =0;

     CheckData=JNI_FALSE;
     CheckAck=JNI_TRUE;
	if (D)  LOGE("ReadUsbData while being");
    size =Java_Java_android_serialport_SerialPort_ReadComData(bufferRev);
	if (size <=0 || size >1024) return 0;
	memcpy(OutbufferRev,Readbuffer,size);
	if (D)  LOGE("ReadUsbData while  end size :%d",size);
	return size;
}


JNIEXPORT jint JNICALL Java_Java_android_serialport_SerialPort_SendCmd
(JNIEnv *env, jobject thiz, jbyte inputdata[],jint nlen,jboolean ReadType,jbyteArray bufferRev)
{
	  jint size=0;
	  jint nread;
	  jint npos =14;
	  jbyte SendBuf[npos + nlen +5];
	  jbyte redBuf[nMaxDatalen];
	  jint ret;
	  SendBuf[0] =CMD_PACK_HEAD;
	  SendBuf[1] =(jbyte)((npos+nlen)/256);
	  SendBuf[2] =(jbyte)((npos+nlen)%256);
	  //以下是8个字节密钥
	  SendBuf[3] =0x00;
	  SendBuf[4] =0x00;
	  SendBuf[5] =0x00;
	  SendBuf[6] =0x00;
	  SendBuf[7] =0x00;
	  SendBuf[8] =0x00;
	  SendBuf[9] =0x00;
	  SendBuf[10] =0x00;
	  //MAC密钥索引
	  SendBuf[11] =0x01;
	  //加密索引
	  SendBuf[12] =0x01;


	  //假设5G
	 // 19   5
	  memcpy(SendBuf+13,inputdata,nlen);


    //19
	  //MAC校验4个字节
	  SendBuf[npos+nlen-1] =0x00;
	  SendBuf[npos+nlen] =0x00;
	  SendBuf[npos+nlen+1] =0x00;
	  SendBuf[npos+nlen+2] =0x00;

	  SendBuf[npos+nlen+3] = (jbyte)Java_android_serialport_SerialPort_Checksum(SendBuf, npos+nlen +2);
	  SendBuf[npos+nlen+4] = CMD_PACK_END;
      bzero(Readbuffer, sizeof(Readbuffer));
      Readbytes =0;
	  CheckAck =JNI_FALSE;
	  CheckData =JNI_FALSE;
	  OperWrite =JNI_TRUE;
	  if (D)  LOGE("write data size =%d",npos +nlen+5);
	  sleep(0.1);
      ret =write(mTtyfd, SendBuf, npos + nlen +5);
      if (D)  LOGE("write data ret =%d",ret);
	   if(D)
  	    {

  	      for (nread =0;nread <npos + nlen +5;nread++)
  	      {
  	    	  if(D)LOGE("write data i =%d,%x",nread,SendBuf[nread] &0xff);
  	      }

  	    }

      if (ret<=0)   //写入失败
   	  {
    	  OperWrite =JNI_FALSE;
  	      return -1;
      }
      if (!ReadType)
      {
			if (!Java_Java_android_serialport_SerialPort_ReadCheckUsbData(SendBuf))  //收到ACK失败返回0
			{
				 OperWrite =JNI_FALSE;
				 return 0;
			}
		  OperWrite =JNI_FALSE;
     	   return 1;  //收到ACK成功 返回1
      }
      ReadTotalbytes =0;
      Readbytes =0;;
      bzero(Readbuffer, sizeof(Readbuffer));
      bzero(redBuf, sizeof(redBuf));
      size =Java_Java_android_serialport_SerialPort_ReadCommData(redBuf,nTimeout);
      if (size==0)  //获取数据失败
      {
    	  OperWrite =JNI_FALSE;
      	 return 0;
      }
      if ((redBuf[0] & 0xFF) !=0x02) return 0;
	  size =redBuf[1]& 0xFF;
	  size<<=8;
	  size|=redBuf[2]& 0xFF;
	  if (D)  LOGE("read size :%d",size +5);
	  if (redBuf[0] == 0x02 && (redBuf[14] & 0xFF) == 0xA0
			  && redBuf[15] == 0x34 && redBuf[16] == 0x00
			  && redBuf[17] == 0x00) {


		      Readbytes =0;;
		      bzero(Readbuffer, sizeof(Readbuffer));
		      bzero(redBuf, sizeof(redBuf));
		      size =Java_Java_android_serialport_SerialPort_ReadCommData(redBuf,nTimeout);
		      if (size==0)  //获取数据失败
		      {
		    	  OperWrite =JNI_FALSE;
		      	 return 0;
		      }
		      if ((redBuf[0] & 0xFF) !=0x02) return 0;
			  size =redBuf[1]& 0xFF;
			  size<<=8;
			  size|=redBuf[2]& 0xFF;

	 }


	  //处理数据,020019 0000000000000000  01   000A  A231  0000 0004 A83FA66A  F0F0F0F0  DE 03

	  // ndatasize =redBuf[1]& 0xFF;
	  // ndatasize<<=8;
	  // ndatasize|=redBuf[2]& 0xFF;

	  // CallBackData(redBuf,size +5);
	   if (redBuf[16] ==0x00 && redBuf[17] ==0x00)  //如果操作代码成功
	   {
		   size =size -17; //8个字节随机数８字节 +项目１字节 +大小2字节+操作代码2字节 +MAC4字节
		    (*env)->SetByteArrayRegion(env,bufferRev,0,size,redBuf+16);
		//    CallBackData(redBuf+16,size);

	   }else
	   {
		   size =2;
		   (*env)->SetByteArrayRegion(env,bufferRev,0,2,redBuf+16);
		  // CallBackData(redBuf+16,size);
	   }

	//  memcpy(bufferRev,redBuf,size+5);
	  OperWrite =JNI_FALSE;

	  return size;
}



/*
 * 线程Run
 */
void* threadreadTtyData(void *args){



    char Revicebuf[nMaxDatalen];


    int result = 0,ret;
    fd_set readfd;
    struct timeval timeout;
    while(mOpen){

       if (OperWrite) //正在操作功能,留给应用
       {
    	   sleep(0.01);
    	   //if(D)LOGE ("mTtyfd OperWrite");
    	   continue;

       }
       timeout.tv_sec = 0;//设定超时秒数
       timeout.tv_usec = 80;//设定超时毫秒数
       memset(Revicebuf,0x00,nMaxDatalen);
       FD_ZERO(&readfd);//清空集合
       FD_SET(mTtyfd,&readfd);///* 把要检测的句柄mTtyfd加入到集合里 */
       ret = select(mTtyfd+1,&readfd,NULL,NULL,&timeout);/* 检测我们上面设置到集合readfd里的句柄是否有可读信息 */
       //if(D)LOGE("thread select ret: %d", ret);
       switch(ret){
       case -1:/* 这说明select函数出错 */
           result = -1;
           if(D)LOGE ("mTtyfd read failure");
           break;
       case 0:/* 说明在我们设定的时间值5秒加0毫秒的时间内，mTty的状态没有发生变化 */
           break;
       default:/* 说明等待时间还未到5秒加0毫秒，mTty的状态发生了变化 */
              if(FD_ISSET(mTtyfd,&readfd)){/* 先判断一下mTty这外被监视的句柄是否真的变成可读的了 */
              int bytes = read(mTtyfd,Revicebuf,sizeof(Revicebuf));
              /**发送数据**/
               if(D)LOGE("ReviceDATA: %d", bytes);
       	       if(D)
         	    {
       	    	  int nread=0;
         	      for (nread =0;nread <bytes;nread++)
         	      {
         	    	 // str =str +spintf("%x",Revicebuf[nread] &0xff);
         	    	  if(D)LOGE("read data i =%d,%x",nread,Revicebuf[nread] &0xff);
         	      }

         	    }
       	       if (bytes==1)
       	      {
       	            continue;
       	        }
       	       if (bytes >0)
       	       {
       	    	  if (D) LOGE("READ  read ReadTotalbytes:%d", ReadTotalbytes);
       	    	  memcpy(Readbuffer+ReadTotalbytes,Revicebuf,bytes);
       	    	  ReadTotalbytes =ReadTotalbytes+bytes;
       	    	  if ((Readbuffer[0]&0xff) ==0x02)
       	          {

       	                    		  nDatalen = Readbuffer[1] & 0xFF;
       	                    		  nDatalen <<= 8;
       	                    		  nDatalen |= Readbuffer[2] & 0xFF;	  //需要接收的总的大小
       	                    		   if (D) LOGE("READ nlen :", nDatalen);
       	                    		   if (nDatalen>0 && nDatalen <512)
       	                    			  Readbytes =nDatalen +5;
       	         }

       	    	 if (D) LOGE("READ  read ReadTotalbytes:%d,%d", ReadTotalbytes,Readbytes);
       	    	 if ((ReadTotalbytes >=Readbytes-1) && ReadTotalbytes >5){

       		          if (ReadTotalbytes +1== Readbytes)
       			        {
       			        	    	  Readbuffer[Readbytes-1] =0x03;
       			       }
       			        ReadTotalbytes =0;
       			        if (!CheckAck)
       			        {
       			        	     if (D) LOGE("READ buffer CheckAck true");
       			        	     CheckAck =JNI_TRUE;
       			        	     CheckData =JNI_FALSE;
       			        	     continue;
       			        }

       			        if (!CheckData) {
       			            CheckData=JNI_TRUE;
       			            if (D) LOGE("READ buffer CheckData true");
       	        	        continue;
       			      }
       	    	 }

       	       }
           }
           continue;
       }
       if(result == -1){
           break;
       }
    }
    if(D)LOGE ("stop run!");
    return NULL;
}

JNIEXPORT jobject JNICALL Java_android_serialport_SerialPort_open
(JNIEnv *env, jobject thiz, jstring path, jint baudrate,jint flags)

{

	jobject mFileDescriptor;


	speed_t speed;


     if(D)LOGD("open() path = %d", path);
	/* Check arguments */
	{
		speed = getBaudrate(baudrate);
		if (speed == -1) {
			/* TODO: throw an exception */
			 if(D)LOGE("Invalid baudrate");
			return NULL;
		}

	}

	/* Opening device */
	{

		jboolean iscopy;
		const char *path_utf = (*env)->GetStringUTFChars(env, path, &iscopy);
		LOGD("Opening serial port %s with flags 0x%x", path_utf, O_RDWR | flags);
		mTtyfd = open(path_utf, O_RDWR | flags);
		 if(D)LOGD("open() fd = %d", mTtyfd);
		(*env)->ReleaseStringUTFChars(env, path, path_utf);

		if (mTtyfd == -1)
		{
			/* Throw an exception */
			 if(D)LOGE("Cannot open port");
			/* TODO: throw an exception */
			return NULL;
		}




	}

	/* Configure device */
	{
		struct termios cfg;

		LOGD("Configuring serial port");
		if (tcgetattr(mTtyfd, &cfg))
		{
			 if(D)LOGE("tcgetattr() failed");
			close(mTtyfd);
			/* TODO: throw an exception */
			return NULL;
		}

		cfmakeraw(&cfg);
		cfsetispeed(&cfg, speed);
		cfsetospeed(&cfg, speed);
		cfg.c_cc[VTIME] = 0; /* 设置超时 15 seconds*/
		cfg.c_cc[VMIN] = 0;
		if (tcsetattr(mTtyfd, TCSANOW, &cfg))
		{
			 if(D)LOGE("tcsetattr() failed");
			close(mTtyfd);
			/* TODO: throw an exception */
			return NULL;
		}
	}


	/* Create a corresponding file descriptor */
	{
		jclass cFileDescriptor = (*env)->FindClass(env, "java/io/FileDescriptor");
		jmethodID iFileDescriptor = (*env)->GetMethodID(env, cFileDescriptor, "<init>", "()V");
		jfieldID descriptorID = (*env)->GetFieldID(env, cFileDescriptor, "descriptor", "I");
		mFileDescriptor = (*env)->NewObject(env, cFileDescriptor, iFileDescriptor);
		(*env)->SetIntField(env, mFileDescriptor, descriptorID, (jint)mTtyfd);
	}

	mOpen =1;
	jenv =env;
    pthread_t id;
    int ret;
     ret = pthread_create(&id,NULL,threadreadTtyData,NULL);
       if(ret != 0){
           if(D)LOGE("create receiver thread failure");
     }else{
           if(D)LOGE("create read data thred success");
     }
      return mFileDescriptor;
}

JNIEXPORT void JNICALL Java_android_serialport_SerialPort_set_1speed
  (JNIEnv *env, jobject obj, jint fd, jint speed)
{
    int i;
    int status;
    struct termios Opt;
    tcgetattr(fd, &Opt);
    for(i=0; i<sizeof(speed_arr)/sizeof(int); i++)
    {
        if(speed == name_arr[i])
        {
            tcflush(fd, TCIOFLUSH);
            cfsetispeed(&Opt, speed_arr[i]);
            cfsetospeed(&Opt, speed_arr[i]);
            status = tcsetattr(fd, TCSANOW, &Opt);
            if(status != 0)
                perror("tcsetattr fd1\n");
            return ;
        }
        tcflush(fd, TCIOFLUSH);
    }
}

JNIEXPORT jint JNICALL Java_android_serialport_SerialPort_set_1Parity
  (JNIEnv *env, jobject obj, jint fd, jint databits, jint stopbits, jint parity)
{
    struct termios opt;
    if(tcgetattr(fd, &opt) != 0)
    {
        perror("SetupSerial 1\n");
        return -1;
    }
    opt.c_cflag &= ~CSIZE;
    opt.c_lflag &= ~(ICANON|ECHO|ECHOE|ISIG);
    opt.c_oflag &= ~OPOST;

    switch(databits)
    {
        case 7: opt.c_cflag |= CS7; break;
        case 8: opt.c_cflag |= CS8; break;
        default: fprintf(stderr, "Unsupported data size\n");
             return -1;
    }
    switch(parity)
    {
        case 'n':
        case 'N': opt.c_cflag &= ~PARENB;
              opt.c_iflag &= ~INPCK;
              break;
        case 'o':
        case 'O': opt.c_cflag |= (PARODD|PARENB);
              opt.c_iflag |= INPCK;
              break;
        case 'e':
        case 'E': opt.c_cflag |= PARENB;
              opt.c_cflag &= ~PARODD;
              opt.c_iflag |= INPCK;
              break;
        case 's':
        case 'S': opt.c_cflag &= ~PARENB;
              opt.c_cflag &= ~CSTOPB;
              break;
        default: fprintf(stderr, "Unsupported parity\n");
             return -1;

    }
    switch(stopbits)
    {
        case 1: opt.c_cflag &= ~CSTOPB;
                           break;
        case 2: opt.c_cflag |= CSTOPB;
            break;
        default: fprintf(stderr,"Unsupported stop bits\n");
             return -1;
    }

    if (parity != 'n')  opt.c_iflag |= INPCK;
//    tcflush(fd,TCIFLUSH);
    opt.c_cc[VTIME] = 150; /* 设置超时 15 seconds*/
    opt.c_cc[VMIN] = 0;
    tcflush(fd, TCIFLUSH);
    if (tcsetattr(fd,TCSANOW,&opt) != 0)
    {
        perror("SetupSerial 3\n");
        return -1;
    }
     return 0;
}



JNIEXPORT jstring JNICALL Java_android_serialport_SerialPort_read
  (JNIEnv *env, jobject obj, jint fd, jint len)
{
    int nread=0;
    char tempBuff[len];
    jstring jstr;
    bzero(tempBuff, sizeof(tempBuff));
    while((nread = read(fd, tempBuff, len))>0)
    {
        tempBuff[nread+1] = '\0';
        jstr = (*env)->NewStringUTF(env, tempBuff);
        return jstr;
    }

}

JNIEXPORT void JNICALL Java_Java_android_serialport_SerialPort_Closeport
  (JNIEnv *env, jobject obj, jint fd)
{
    if(close(fd))
    {
        perror("close failed!\n");
    }
    else
    {
        printf("close success!\n");
    }
}


//以下协议函数








JNIEXPORT int  JNICALL Java_android_serialport_SerialPort_getstate
(JNIEnv *env, jobject thiz,jbyteArray OutData)
{

    if(D)LOGE("Check  getstate begin");
  // char *tmpdata = (char*)(*env)->GetByteArrayElements(env, Indata, NULL);

     jint size =0;
	 jbyte SendBuf[5];
	 SendBuf[0]=CMD_COUNT;
	 SendBuf[1]=0x00;
	 SendBuf[2]=0x02;
	 SendBuf[3]=0xA0;
	 SendBuf[4]=0x33;

   size =Java_Java_android_serialport_SerialPort_SendCmd(env,thiz,SendBuf,5,JNI_TRUE,OutData);
 // (*env)->ReleaseByteArrayElements(env, Indata, tmpdata, 0);

   if (size >0)
   {
	   if(D)LOGE("Check  getstate Sucess,size:%d,data=%s",size,OutData);
   }
   return size;
}



/***
 * 刷卡
 *
 * @param dwatitimeouts
 *        刷卡超时时间秒(16进制)
 * @param OutData
 *         刷卡返回数据

 * 传出的数据
 * @return  int  刷卡返回的数据大小
 */


JNIEXPORT int  JNICALL Java_android_serialport_SerialPort_BrushCard
(JNIEnv *env, jobject thiz,jint dWaitTimeouts,jbyteArray OutData)
{

     if(D)LOGE("BrushCard begin");
     jint size =0;
	 jbyte SendBuf[8];
	 SendBuf[0]=CMD_COUNT;
	 SendBuf[1]=0x00;
	 SendBuf[2]=0x05;
	 SendBuf[3]=0xA6;
	 SendBuf[4]=0x31;
	 SendBuf[5] =(jbyte)((dWaitTimeouts)/256);
	 SendBuf[6] =(jbyte)((dWaitTimeouts)%256);
	 SendBuf[7]=0x00;
    size =Java_Java_android_serialport_SerialPort_SendCmd(env,thiz,SendBuf,8,JNI_TRUE,OutData);
    if (size >0)
   {
	   if(D)LOGE("BrushCard Sucess,size:%d,data=%s",size,OutData);
   }else if(D)LOGE("BrushCard Faild,size:%d",size);
   return size;
}



/***
 *非接寻卡
 *
 * @param dwatitimeouts
 *        寻卡超时时间秒(16进制)
 * @param OutData
 *         寻卡返回数据

 * 传出的数据
 * @return  int   返回的数据大小
  OutData  =状态码(00)+1个字节长度+卡号,状态码不为00,只返回状态码
*/



JNIEXPORT int  JNICALL Java_android_serialport_SerialPort_GetRFCardInfo
(JNIEnv *env, jobject thiz,jint dWaitTimeouts,jbyteArray OutData)
{

     if(D)LOGE("GetRFCardInfo begin");
     jint size =0;
	 jbyte SendBuf[7];
	 SendBuf[0]=CMD_COUNT;
	 SendBuf[1]=0x00;
	 SendBuf[2]=0x04;
	 SendBuf[3]=CMD_RFID;
	 SendBuf[4]=0x31;
	 SendBuf[5] =(jbyte)((dWaitTimeouts)/256);
	 SendBuf[6] =(jbyte)((dWaitTimeouts)%256);
    size =Java_Java_android_serialport_SerialPort_SendCmd(env,thiz,SendBuf,7,JNI_TRUE,OutData);
    if (size >0)
    {
	   if(D)LOGE("GetRFCardInfo Sucess,size:%d,data=%s",size,OutData);
   }else if(D)LOGE("GetRFCardInfo Faild,size:%d",size);
   return size;
}




/***
 *非接发送数据
 *
 * @param inputdata
 *          发送的数据
 *@param  Inlen
              发送的数据大小
  *@param  OutData
              接收到的数据
 * 传出的数据
 * @return  int 接收到的数据大小

*/


JNIEXPORT int  JNICALL Java_android_serialport_SerialPort_OperRfCard
(JNIEnv *env, jobject thiz,jbyteArray InputData,jint InLen,jbyteArray OutData)
{

     if(D)LOGE("OperRfCard begin");
     jint size =0;
	 jbyte SendBuf[7 + InLen];
	 SendBuf[0]=CMD_COUNT;
	 SendBuf[1] =(jbyte)((InLen +2 )/256);
	 SendBuf[2] =(jbyte)((InLen +2)%256);
	 SendBuf[3]=CMD_RFID;
	 SendBuf[4]=0x34;
	 SendBuf[5]=(jbyte)((InLen)/256);
	 SendBuf[6]=(jbyte)((InLen)%256);
     char *chArr = (char*)(*env)->GetByteArrayElements(env,InputData,0);
	 memcpy(SendBuf+7,chArr,InLen);
    size =Java_Java_android_serialport_SerialPort_SendCmd(env,thiz,SendBuf,7 + InLen,JNI_TRUE,OutData);
    if (size >0)
    {
	   if(D)LOGE("OperRfCard Sucess,size:%d,data=%s",size,OutData);
   }else if(D)LOGE("OperRfCard Faild,size:%d",size);
   return size;
}




/***
 *非接断开连接
 *
 * @param dwatitimeouts
 *        断开超时时间秒(16进制)
 * 传出的数据
 * @return  Boolean
             True :断开成功
             False：断开失败
*/


JNIEXPORT int  JNICALL Java_android_serialport_SerialPort_DisconnectRfCard
(JNIEnv *env, jobject thiz,jint dWaitTimeouts)
{

	 jbyteArray OutData = (*env)->NewByteArray(env,128);
     if(D)LOGE("DisconnectRfCard begin");
     jint size =0;
	 jbyte SendBuf[7];
	 SendBuf[0]=CMD_COUNT;
	 SendBuf[1]=0x00;
	 SendBuf[2]=0x04;
	 SendBuf[3]=CMD_RFID;
	 SendBuf[4]=0x32;
	 SendBuf[5] =(jbyte)((dWaitTimeouts)/256);
	 SendBuf[6] =(jbyte)((dWaitTimeouts)%256);
     size =Java_Java_android_serialport_SerialPort_SendCmd(env,thiz,SendBuf,7,JNI_TRUE,OutData);
     if (size >0)
     {
	   if(D)LOGE("DisconnectRfCard Sucess,size:%d,data=%s",size,OutData);
     }else if(D)LOGE("DisconnectRfCard Faild,size:%d",size);
  // return size;
     return 1;
}




/***
 *IC卡复位
 *
 * @param dwatitimeouts
 *        复位超时时间秒(16进制)
 * @param OutData
 *         复位成功返回数据

 * 传出的数据
 * @return  int   返回的数据大小
   OutData  =状态码(00)+1个字节长度+卡号,状态码不为00,只返回状态码
*/


JNIEXPORT int  JNICALL Java_android_serialport_SerialPort_IcCardReset
(JNIEnv *env, jobject thiz,jint dWaitTimeouts,jbyteArray OutData)
{

     if(D)LOGE("IcCardReset begin");
     jint size =0;
	 jbyte SendBuf[7];
	 SendBuf[0]=CMD_COUNT;
	 SendBuf[1]=0x00;
	 SendBuf[2]=0x04;
	 SendBuf[3]=CMD_ICCARD;
	 SendBuf[4]=0x31;
	 SendBuf[5] =(jbyte)((dWaitTimeouts)/256);
	 SendBuf[6] =(jbyte)((dWaitTimeouts)%256);
    size =Java_Java_android_serialport_SerialPort_SendCmd(env,thiz,SendBuf,7,JNI_TRUE,OutData);
    if (size >0)
    {
	   if(D)LOGE("IcCardReset Sucess,size:%d,data=%s",size,OutData);
   }else if(D)LOGE("IcCardReset Faild,size:%d",size);
   return size;
}




/***
 *IC卡下电
 *
 * @param dwatitimeouts
 *        下电超时时间秒(16进制)
 * 传出的数据
 * @return  int  返回状态
              00为状态成功
               01 为失败
*/


JNIEXPORT int  JNICALL Java_android_serialport_SerialPort_DisconnectICard
(JNIEnv *env, jobject thiz,jint dWaitTimeouts)
{

	 jbyteArray OutData = (*env)->NewByteArray(env,128);
     if(D)LOGE("DisconnectRfCard begin");
     jint size =0;
	 jbyte SendBuf[7];
	 SendBuf[0]=CMD_COUNT;
	 SendBuf[1]=0x00;
	 SendBuf[2]=0x04;
	 SendBuf[3]=CMD_ICCARD;
	 SendBuf[4]=0x32;
	 SendBuf[5] =(jbyte)((dWaitTimeouts)/256);
	 SendBuf[6] =(jbyte)((dWaitTimeouts)%256);
     size =Java_Java_android_serialport_SerialPort_SendCmd(env,thiz,SendBuf,7,JNI_TRUE,OutData);
     if (size >0)
     {
	   if(D)LOGE("DisconnectRfCard Sucess,size:%d,data=%s",size,OutData);
     }else if(D)LOGE("DisconnectRfCard Faild,size:%d",size);
  // return size;
     return 1;
}



/***
 *IC发送数据
 *
 * @param inputdata
 *          发送的数据
 *@param  Inlen
              发送的数据大小
  *@param  OutData
              接收到的数据
 * 传出的数据
 * @return  int 接收到的数据大小

*/


JNIEXPORT int  JNICALL Java_android_serialport_SerialPort_OperICCard
(JNIEnv *env, jobject thiz,jbyteArray InputData,jint InLen,jbyteArray OutData)
{

     if(D)LOGE("OperICCard begin");
     jint size =0;
	 jbyte SendBuf[7 + InLen];
	 SendBuf[0]=CMD_COUNT;
	 SendBuf[1] =(jbyte)((InLen +2 )/256);
	 SendBuf[2] =(jbyte)((InLen +2)%256);
	 SendBuf[3]=CMD_ICCARD;
	 SendBuf[4]=0x33;
	 SendBuf[5]=(jbyte)((InLen)/256);
	 SendBuf[6]=(jbyte)((InLen)%256);
     char *chArr = (char*)(*env)->GetByteArrayElements(env,InputData,0);
	 memcpy(SendBuf+7,chArr,InLen);
    size =Java_Java_android_serialport_SerialPort_SendCmd(env,thiz,SendBuf,7 + InLen,JNI_TRUE,OutData);
	 (*env)->ReleaseByteArrayElements(env, InputData, chArr, 0);
    if (size >0)
    {
	   if(D)LOGE("OperICCard Sucess,size:%d,data=%s",size,OutData);
   }else if(D)LOGE("OperICCard Faild,size:%d",size);
   return size;
}









/***
 *PASM卡复位
 *
 * @param OutData
 *         复位成功返回数据

 * 传出的数据
 * @return  int   返回的数据大小
   OutData  =状态码(00)+1个字节长度+卡号,状态码不为00,只返回状态码
*/


JNIEXPORT int  JNICALL Java_android_serialport_SerialPort_PasmReset
(JNIEnv *env, jobject thiz,jbyteArray OutData)
{
     if(D)LOGE("PasmReset begin");
     jint size =0;
	 jbyte SendBuf[5];
	 SendBuf[0]=CMD_COUNT;
	 SendBuf[1]=0x00;
	 SendBuf[2]=0x02;
	 if (pasmcard ==1)
	     SendBuf[3]=CMD_PSAM1;
	 else
		 SendBuf[3]=CMD_PSAM2;

	 SendBuf[4]=0x31;
     size =Java_Java_android_serialport_SerialPort_SendCmd(env,thiz,SendBuf,5,JNI_TRUE,OutData);

     if (size >0)
     {
	   if(D)LOGE("PasmReset Sucess,size:%d,data=%s",size,OutData);
     }else if(D)LOGE("PasmReset Faild,size:%d",size);
      return size;

}






/***
 * PASM上电
命令码	A4H/A5H	A4/A5 32 psam卡下电

	终端上行
字段	长度（byte）	说明
命令码	2字节	A4/A5 32操作卡片数据命令代码
返回码	2字节	0x00,0x00
*/


JNIEXPORT int  JNICALL Java_android_serialport_SerialPort_Pasmpoweroff
(JNIEnv *env, jobject thiz,jbyteArray OutData)
{
     if(D)LOGE("Pasmpoweroff begin");
     jint size =0;
	 jbyte SendBuf[5];
	 SendBuf[0]=CMD_COUNT;
	 SendBuf[1]=0x00;
	 SendBuf[2]=0x02;
	 if (pasmcard ==1)
	     SendBuf[3]=CMD_PSAM1;
	 else
		 SendBuf[3]=CMD_PSAM2;
	 SendBuf[4]=0x32;
     size =Java_Java_android_serialport_SerialPort_SendCmd(env,thiz,SendBuf,5,JNI_TRUE,OutData);
     if (size >0)
     {
	   if(D)LOGE("Pasmpoweroff Sucess,size:%d,data=%s",size,OutData);
     }else if(D)LOGE("Pasmpoweroff Faild,size:%d",size);
     return size;
}




/***
 *PASM发送数据
 *
 * @param inputdata
 *          发送的数据
 *@param  Inlen
              发送的数据大小
  *@param  OutData
              接收到的数据
 * 传出的数据
 * @return  int 接收到的数据大小

*/

JNIEXPORT int  JNICALL Java_android_serialport_SerialPort_OperPasmCard
(JNIEnv *env, jobject thiz,jbyteArray InputData,jint InLen,jbyteArray OutData)
{

     if(D)LOGE("OperPasmCard begin");
     jint size =0;
	 jbyte SendBuf[7 + InLen];
	 SendBuf[0]=CMD_COUNT;
	 SendBuf[1] =(jbyte)((InLen +2 )/256);
	 SendBuf[2] =(jbyte)((InLen +2)%256);

	 if (pasmcard ==1)
	     SendBuf[3]=CMD_PSAM1;
	 else
		 SendBuf[3]=CMD_PSAM2;
	 SendBuf[4]=0x33;
	 SendBuf[5]=(jbyte)((InLen)/256);
	 SendBuf[6]=(jbyte)((InLen)%256);
     char *chArr = (char*)(*env)->GetByteArrayElements(env,InputData,0);
	 memcpy(SendBuf+7,chArr,InLen);
    size =Java_Java_android_serialport_SerialPort_SendCmd(env,thiz,SendBuf,7 + InLen,JNI_TRUE,OutData);
	 (*env)->ReleaseByteArrayElements(env, InputData, chArr, 0);
    if (size >0)
    {
	   if(D)LOGE("OperPasmCard Sucess,size:%d,data=%s",size,OutData);
   }else if(D)LOGE("OperPasmCard Faild,size:%d",size);
   return size;
}




/*
 *
 *  M1卡相关API函数
 *
 */




/*
 * M1寻卡
 * CommandH	A7H	卡片操作命令类别
	CommandL	31H	连接RFID-SIM卡命令代码
	DelayTime	2字节	等待卡进入感应区时间，高位在前，低位在后。为0时：感应区无卡直接返回失败；为0xffff时,一直寻卡，直到卡进入感应区；其它值时：在DelayTime毫秒时间内一直判断卡是否进入感应区
 *Status	00H	00H	连接成功
	A0H	01H	RFID-SIM卡未连接
	A0H	05H	RFID-SIM卡连接失败
	A0H	06H	等待卡进入感应区超时
UIDLen	1字节	卡序列号长度
Card UID	UIDLen字节	卡序列号（连接成功才返回）
 *
 */


JNIEXPORT int  JNICALL Java_android_serialport_SerialPort_GetM1CardInfo
(JNIEnv *env, jobject thiz,jint dWaitTimeouts,jbyteArray OutData)
{

     if(D)LOGE("GetRFCardInfo begin");
     jint size =0;
	 jbyte SendBuf[7];
	 SendBuf[0]=CMD_COUNT;
	 SendBuf[1]=0x00;
	 SendBuf[2]=0x04;
	 SendBuf[3]=CMD_M1;
	 SendBuf[4]=0x31;
	 SendBuf[5] =(jbyte)((dWaitTimeouts)/256);
	 SendBuf[6] =(jbyte)((dWaitTimeouts)%256);
    size =Java_Java_android_serialport_SerialPort_SendCmd(env,thiz,SendBuf,7,JNI_TRUE,OutData);
    if (size >0)
    {
	   if(D)LOGE("GetRFCardInfo Sucess,size:%d,data=%s",size,OutData);
   }else if(D)LOGE("GetRFCardInfo Faild,size:%d",size);
   return size;
}




/*
 /*
  * 2.认证密钥
  * CommandH	A7H	M1卡片操作命令类别
	CommandL	32H	认证密钥命令
	Key_type	1字节	密码验证模式；0表示A密码（KEYA）,1表示B密码（KEYB）
	Sector	1字节	要验证密码的扇区号（0～15）
	snr	4字节	卡号，mifare1C长度4byte

	返回
	标识	内容	说明
	Status	00H	00H	验证成功
	A0H	01H	RFID-SIM卡未连接
	A0H	05H	验证失败
	A0H	06H	参数错误
  *
  */



JNIEXPORT int  JNICALL Java_android_serialport_SerialPort_M1AuthKey
(JNIEnv *env, jobject thiz,jbyte Key_type,jbyte Sector,jbyteArray InputSnData,jint InSnLen,jbyteArray InputKeyData,jint InKeyLen,jbyteArray OutData)
{


    if(D)LOGE("M1AuthKey begin");
    jint size =0;
	 jbyte SendBuf[7 + InSnLen + InKeyLen];
	 SendBuf[0]=CMD_COUNT;
	 SendBuf[1] =(jbyte)((InSnLen +4 +InKeyLen )/256);
	 SendBuf[2] =(jbyte)((InSnLen +4 + InKeyLen)%256);
	 SendBuf[3]=CMD_M1;
	 SendBuf[4]=0x32;
	 SendBuf[5]=Key_type;
	 SendBuf[6]=Sector;
     char *chsnArr = (char*)(*env)->GetByteArrayElements(env,InputSnData,0);
     memcpy(SendBuf+7,chsnArr,InSnLen);
     char *chkeyArr = (char*)(*env)->GetByteArrayElements(env,InputKeyData,0);
     memcpy(SendBuf+7 +InSnLen ,chkeyArr,InKeyLen);
     size =Java_Java_android_serialport_SerialPort_SendCmd(env,thiz,SendBuf,7 + InSnLen + InKeyLen,JNI_TRUE,OutData);
	 (*env)->ReleaseByteArrayElements(env, InputSnData, chsnArr, 0);
	 (*env)->ReleaseByteArrayElements(env, InputKeyData, chkeyArr, 0);
      if (size >0)
     {
  	   if(D)LOGE("M1AuthKey Sucess,size:%d,data=%s",size,OutData);
      }else if(D)LOGE("M1AuthKey Faild,size:%d",size);
  return size;
}


/*
 * MI读卡
 * 请求
标识	内容	说明
CommandH	A7H	M1卡片操作命令类别
CommandL	33H	读命令
Sector	1字节	要验证密码的扇区号（0～15）

返回
标识	内容	说明
Status	00H	00H	读取成功
	A0H	01H	RFID-SIM卡未连接
	A0H	05H	读取失败
	A0H	06H	参数错误
返回数据	16字节	当成功时有数据
 *
 *
 *
 */



 JNIEXPORT int  JNICALL Java_android_serialport_SerialPort_ReadM1CardData
 (JNIEnv *env, jobject thiz,jbyte kesec,jbyteArray OutData)
 {
     if(D)LOGE("ReadM1CardData begin");
     jint size =0;
	 jbyte SendBuf[6];
	 SendBuf[0]=CMD_COUNT;
	 SendBuf[1]=0x00;
	 SendBuf[2]=0x03;
	 SendBuf[3]=CMD_M1;
	 SendBuf[4]=0x33;
	 SendBuf[5]=kesec;
     size =Java_Java_android_serialport_SerialPort_SendCmd(env,thiz,SendBuf,6,JNI_TRUE,OutData);
    if (size >0)
    {
	   if(D)LOGE("ReadM1CardData Sucess,size:%d,data=%s",size,OutData);
    }else if(D)LOGE("ReadM1CardData Faild,size:%d",size);
     return size;
 }




 /*
  * 4.写数据
请求
标识	内容	说明
CommandH	A7H	M1卡片操作命令类别
CommandL	34H	写命令
Sector	1字节	要验证密码的扇区号（0～15）
写入数据	16字节	要写入的数据

返回
标识	内容	说明
Status	00H	00H	写入成功
	A0H	01H	RFID-SIM卡未连接
	A0H	05H	写入失败
	A0H	06H	参数错误
  *
  */

 JNIEXPORT int  JNICALL Java_android_serialport_SerialPort_WriteM1CardData(JNIEnv *env, jobject thiz,jbyte kesec,jbyteArray InputData,int  InLen,jbyteArray OutData)
 {

     if(D)LOGE("WriteM1CardData begin");
     jint size =0;
	 jbyte SendBuf[6 + InLen];
	 SendBuf[0]=CMD_COUNT;
	 SendBuf[1] =(jbyte)((InLen +3 )/256);
	 SendBuf[2] =(jbyte)((InLen +3)%256);
	 SendBuf[3]=CMD_M1;
	 SendBuf[4]=0x34;
	 SendBuf[5]=kesec;
     char *chArr = (char*)(*env)->GetByteArrayElements(env,InputData,0);
	 memcpy(SendBuf+6,chArr,InLen);
     size =Java_Java_android_serialport_SerialPort_SendCmd(env,thiz,SendBuf,6 + InLen,JNI_TRUE,OutData);
	 (*env)->ReleaseByteArrayElements(env, InputData, chArr, 0);
    if (size >0)
    {
	   if(D)LOGE("WriteM1CardData Sucess,size:%d,data=%s",size,OutData);
   }else if(D)LOGE("WriteM1CardData Faild,size:%d",size);
   return size;

 }


 /*
  * 5.增值
请求
标识	内容	说明
CommandH	A7H	M1卡片操作命令类别
CommandL	35H	增值命令
Sector	1字节	要验证密码的扇区号（0～15）
value	4字节	增值数据

返回
标识	内容	说明
Status	00H	00H	操作成功
	A0H	01H	RFID-SIM卡未连接
	A0H	05H	操作失败
  *
  */


 JNIEXPORT int  JNICALL Java_android_serialport_SerialPort_AddM1CardData(JNIEnv *env, jobject thiz,jbyte kesec,jbyteArray InputData,int  InLen,jbyteArray OutData)
 {

     if(D)LOGE("AddM1CardData begin");
     jint size =0;
	 jbyte SendBuf[6 + InLen];
	 SendBuf[0]=CMD_COUNT;
	 SendBuf[1] =(jbyte)((InLen +3 )/256);
	 SendBuf[2] =(jbyte)((InLen +3)%256);
	 SendBuf[3]=CMD_M1;
	 SendBuf[4]=0x35;
	 SendBuf[5]=kesec;
     char *chArr = (char*)(*env)->GetByteArrayElements(env,InputData,0);
	 memcpy(SendBuf+6,chArr,InLen);
     size =Java_Java_android_serialport_SerialPort_SendCmd(env,thiz,SendBuf,6 + InLen,JNI_TRUE,OutData);
	 (*env)->ReleaseByteArrayElements(env, InputData, chArr, 0);
    if (size >0)
    {
	   if(D)LOGE("AddM1CardData Sucess,size:%d,data=%s",size,OutData);
   }else if(D)LOGE("AddM1CardData Faild,size:%d",size);
   return size;

 }


 /*
  * 6.减值
请求
标识	内容	说明
CommandH	A7H	M1卡片操作命令类别
CommandL	36H	减值命令
Sector	1字节	要验证密码的扇区号（0～15）
value	4字节	减值数据

返回
标识	内容	说明
Status	00H	00H	操作成功
	A0H	01H	RFID-SIM卡未连接
	A0H	05H	操作失败
	A0H	06H	参数错误

  *
  */



 JNIEXPORT int  JNICALL Java_android_serialport_SerialPort_IncM1CardData
 (JNIEnv *env, jobject thiz,jbyte kesec,jbyteArray InputData,int  InLen,jbyteArray OutData)
 {
     if(D)LOGE("IncM1CardData begin");
     jint size =0;
	 jbyte SendBuf[6 + InLen];
	 SendBuf[0]=CMD_COUNT;
	 SendBuf[1] =(jbyte)((InLen +3 )/256);
	 SendBuf[2] =(jbyte)((InLen +3)%256);
	 SendBuf[3]=CMD_M1;
	 SendBuf[4]=0x36;
	 SendBuf[5]=kesec;
     char *chArr = (char*)(*env)->GetByteArrayElements(env,InputData,0);
	 memcpy(SendBuf+6,chArr,InLen);
     size =Java_Java_android_serialport_SerialPort_SendCmd(env,thiz,SendBuf,6 + InLen,JNI_TRUE,OutData);
	 (*env)->ReleaseByteArrayElements(env, InputData, chArr, 0);
    if (size >0)
    {
	   if(D)LOGE("IncM1CardData Sucess,size:%d,data=%s",size,OutData);
   }else if(D)LOGE("IncM1CardData Faild,size:%d",size);
   return size;
 }


 /*
  * 7.Restore
	请求
	标识	内容	说明
	CommandH	A7H	M1卡片操作命令类别
	CommandL	37H	回传函数，将EEPROM中的内容传入卡的内部寄存器
	Sector	1字节	要验证密码的扇区号（0～15）

	返回
	标识	内容	说明
	Status	00H	00H	操作成功
		A0H	01H	RFID-SIM卡未连接
		A0H	05H	操作失败
  *
  *
  */

 JNIEXPORT int  JNICALL Java_android_serialport_SerialPort_RestoreM1CardData
 (JNIEnv *env, jobject thiz,jbyte kesec,jbyteArray OutData)
 {
     if(D)LOGE("RestoreM1CardData begin");
     jint size =0;
	 jbyte SendBuf[6];
	 SendBuf[0]=CMD_COUNT;
	 SendBuf[1] =0x00;
	 SendBuf[2] =0x03;
	 SendBuf[3]=CMD_M1;
	 SendBuf[4]=0x37;
	 SendBuf[5]=kesec;
     size =Java_Java_android_serialport_SerialPort_SendCmd(env,thiz,SendBuf,6,JNI_TRUE,OutData);
    if (size >0)
    {
	   if(D)LOGE("RestoreM1CardData Sucess,size:%d,data=%s",size,OutData);
   }else if(D)LOGE("RestoreM1CardData Faild,size:%d",size);
   return size;
 }




 /*
  *  退卡
  * 标识	内容	说明
	CommandH	A7H	卡片操作命令类别
	CommandL	38H	断开连接命令代码
	DelayTime	2字节	等待卡拿离感应区时间，高位在前，低位在后。参数说明见备注。

 返回
标识	内容	说明
Status	00H	00H	命令执行正确
	A0H	01H	卡未连接
		06H	等待卡拿离感应区超时
		04H	断开连接失败
  *
  */

 JNIEXPORT int  JNICALL Java_android_serialport_SerialPort_DisconnectM1Card
 (JNIEnv *env, jobject thiz,jint  dWaitTimeouts,jbyteArray OutData)
 {

     if(D)LOGE("DisconnectM1Card begin");
     jint size =0;
	 jbyte SendBuf[7];
	 SendBuf[0]=CMD_COUNT;
	 SendBuf[1] =0x00;
	 SendBuf[2] =0x04;
	 SendBuf[3]=CMD_M1;
	 SendBuf[4]=0x38;
	 SendBuf[5] =(jbyte)((dWaitTimeouts)/256);
	 SendBuf[6] =(jbyte)((dWaitTimeouts)%256);
     size =Java_Java_android_serialport_SerialPort_SendCmd(env,thiz,SendBuf,7,JNI_TRUE,OutData);
    if (size >0)
    {
	   if(D)LOGE("DisconnectM1Card Sucess,size:%d,data=%s",size,OutData);
   }else if(D)LOGE("DisconnectM1Card Faild,size:%d",size);
   return size;
 }




 /*
  * 9.读取e2prom
 	标识	内容	说明
 	CommandH	A7H	M1卡片操作命令类别
 	CommandL	39H	读取e2命令
 	Addr	2字节hex	读取的地址
 	Len	1字节hex	读取的长度
 	请求

 	返回
 	标识	内容	说明
 	Status	00H	00H	操作成功
 		A0H	01H	RFID-SIM卡未连接
 		A0H	05H	操作失败
 		A0H	06H	参数错误
 		Data_len	读出的数据	操作成功才有此域
  *
  */



JNIEXPORT int  JNICALL Java_android_serialport_SerialPort_ReadM1Epprom
(JNIEnv *env, jobject thiz,jint  Address,jint nreadlen,jbyteArray OutData)
 {
    if(D)LOGE("ReadM1Epprom begin");
    jint size =0;
	 jbyte SendBuf[7];
	 SendBuf[0]=CMD_COUNT;
	 SendBuf[1] =0x00;
	 SendBuf[2] =0x04;
	 SendBuf[3]=CMD_M1;
	 SendBuf[4]=0x39;
	 SendBuf[5] =Address;
	 SendBuf[6] =nreadlen;
    size =Java_Java_android_serialport_SerialPort_SendCmd(env,thiz,SendBuf,7,JNI_TRUE,OutData);
   if (size >0)
   {
	   if(D)LOGE("ReadM1Epprom Sucess,size:%d,data=%s",size,OutData);
  }else if(D)LOGE("ReadM1Epprom Faild,size:%d",size);
  return size;
 }





/*
10.写入e2prom
标识	内容	说明
	 CommandH	A7H	M1卡片操作命令类别
	 CommandL	3aH	写入e2命令
	 Addr	2字节hex	写入的地址
	 Len	1字节hex	写入的长度
	 Data 	Len 字节数据	写入的数据
返回
	 标识	内容	说明
	 Status	00H	00H	操作成功
	A0H	01H	RFID-SIM卡未连接
	A0H	05H	操作失败
	A0H	06H	参数错误


 */

JNIEXPORT int  JNICALL Java_android_serialport_SerialPort_WriteM1Epprom
(JNIEnv *env, jobject thiz,int  Address,jbyteArray InputData,jbyte InLen,jbyteArray OutData)
{

	 if(D)LOGE("WriteM1Epprom begin");
	 jint size =0;
	 jbyte SendBuf[7+ InLen];
	 SendBuf[0]=CMD_COUNT;
	 SendBuf[1] =(jbyte)((InLen +3 )/256);
	 SendBuf[2] =(jbyte)((InLen +3)%256);
	 SendBuf[3]=CMD_M1;
	 SendBuf[4]=0x3A;
	 SendBuf[5]=Address;
	 SendBuf[6]=InLen;
	 char *chArr = (char*)(*env)->GetByteArrayElements(env,InputData,0);
	 memcpy(SendBuf+7,chArr,InLen);
	 size =Java_Java_android_serialport_SerialPort_SendCmd(env,thiz,SendBuf,7 + InLen,JNI_TRUE,OutData);
	 (*env)->ReleaseByteArrayElements(env, InputData, chArr, 0);
	 if (size >0)
	 {
	   if(D)LOGE("WriteM1Epprom Sucess,size:%d,data=%s",size,OutData);
	 }else if(D)LOGE("WriteM1Epprom Faild,size:%d",size);
	  return size;

}


