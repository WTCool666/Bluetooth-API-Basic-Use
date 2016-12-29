/*
 * Copyright 2009 Cedric Priscal
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

#define TTY_DEVICE "/dev/ttyS2"

#define RECEIVE_DATA_INDEX (1)

#define POST_EVENT()
static int mTtyfd = -1;
static int mOpen = 0;




static  int  CMD_PACK_HEAD = 0x02;
static  int  CMD_PACK_END = 0x03;
static  int  CMD_RFCARD  = 0xA2;
static  int  CMD_ICCARD  = 0xA3;
static  int  CMD_PASMCARD1 = 0xA4;
static  int  CMD_PASMCARD2 = 0xA5;
static  int  CMD_UPDATE = 0xA1;
static  int  dWaitTimeouts =1000;



static const char *TAG="com_pay_uart_port";
#define LOGI(fmt, args...) __android_log_print(ANDROID_LOG_INFO,  TAG, fmt, ##args)
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, TAG, fmt, ##args)
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, TAG, fmt, ##args)

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
 * Class:     cedric_serial_SerialPort
 * Method:    open
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT jobject JNICALL Java_android_serialport_SerialPort_open
  (JNIEnv *env, jobject thiz, jstring path, jint baudrate,jint flags)
{
    #波特率
	speed_t speed;

	jobject mFileDescriptor;

	/* Check arguments */
	{
		speed = getBaudrate(baudrate);
		if (speed == -1) {
			/* TODO: throw an exception */
			LOGE("Invalid baudrate");
			return NULL;
		}

	}

	/* Opening device */
	{

		jboolean iscopy;
		const char *path_utf = (*env)->GetStringUTFChars(env, path, &iscopy);
		LOGD("Opening serial port %s with flags 0x%x", path_utf, O_RDWR | flags);
		mTtyfd = open(path_utf, O_RDWR | flags);
		LOGD("open() fd = %d", mTtyfd);
		(*env)->ReleaseStringUTFChars(env, path, path_utf);

		if (mTtyfd == -1)
		{
			/* Throw an exception */
			LOGE("Cannot open port");
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
			LOGE("tcgetattr() failed");
			close(mTtyfd);
			/* TODO: throw an exception */
			return NULL;
		}

		cfmakeraw(&cfg);
		cfsetispeed(&cfg, speed);
		cfsetospeed(&cfg, speed);

		if (tcsetattr(mTtyfd, TCSANOW, &cfg))
		{
			LOGE("tcsetattr() failed");
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


	return mFileDescriptor;
}

/*
 * Class:     cedric_serial_SerialPort
 * Method:    close
 * Signature: ()V
 */
JNIEXPORT int JNICALL Java_android_serialport_SerialPort_close
  (JNIEnv *env, jobject thiz)
{
	 LOGI("com_uart_manager_TtyControl__closeTty");
	    if(mTtyfd < 0){
	       LOGE("mTtyfd open failure ,non't close");
	       return -1;
	    }
	    mOpen = 0;
	    sleep(2);//等待线程退出
	    int c = close(mTtyfd);
	    if(c < 0){
	       LOGE("mTtyfd close failure");
	       return -1;
	    }
	    LOGI("close device success");
	    return 1;
}



JNIEXPORT int  JNICALL Java_android_serialport_SerialPort_read(JNIEnv *env, jobject obj,jbyte buf[],int nsize)
{
    int nread=0;
    //char tempBuff[nsize];
    //jstring jstr;
   // bzero(tempBuff, sizeof(tempBuff));
    LOGI("SerialPort_read");
    nread = read(mTtyfd, buf, nsize);
    if (nread >0)  LOGI("SerialPort_read Sucess");
    return nread;
    /*
    while((nread = read(mTtyfd, buf, 256))>0)
    {
        tempBuff[nread+1] = '\0';
        jstr = (*env)->NewStringUTF(env, tempBuff);
	 nsize =nread;
	 LOGI("SerialPort_read Sucess %s",jstr); 
        return jstr;
    }
    */
	LOGI("SerialPort_read Fail");
}



JNIEXPORT int JNICALL Java_android_serialport_SerialPort_WriteData(JNIEnv *env, jobject thiz,jbyte data[])
 {
    	    LOGI("Java_android_serialport_SerialPort_WriteData");
    	     //jbyte * arrayBody = env->GetByteArrayElements(data,0); jsize theArrayLengthJ = env->GetArrayLength(data); BYTE * starter = (BYTE *)arrayBody;
    	    if(mTtyfd < 0){
    	       LOGE("mTtyfd open failure ,non't write");
    	       return -1;
    	    }

    	    jbyte* arrayData =(*env)->GetByteArrayElements(env,data,0);
    	    jsize arrayLength = (*env)->GetArrayLength(env,data);
    	    char* byteData = (char*)arrayData;
    	    int len = (int)arrayLength;
    	    LOGI("write data len:%d",len);
    	    int re = write(mTtyfd,byteData,len);
    	    if(re == -1){
    	       LOGE("write device error");
    	    }
    	  LOGI("write device sucess");
    	 // Java_android_serialport_SerialPort_ReviceData(env,thiz);
    	  return re;
    }

int Java_android_serialport_SerialPort_Checksum(jbyte data[],int nLen)
{
	int chksum,i;
	chksum=0;
	//传过来的时候第一位不计算
	for(i=0+1;i<nLen+1;)
	{
		chksum ^=data[i++];
	}
	chksum&=0x00ff;
	//得到校验和
	return chksum;
}


int Java_android_serialport_SerialPort_WriteBuf(jbyte SendBuf[],int nlen)
{
            LOGI("write data len:%d",nlen);
    	    int re = write(mTtyfd,SendBuf,nlen);
    	    if(re == -1){
    	       LOGE("write device error");
    	    }
    	    LOGI("write device sucess");
    	    return re;
}



JNIEXPORT void JNICALL Java_android_serialport_SerialPort_GetCardInfo(JNIEnv *env, jobject thiz)
 {
   // int nlen =9;
	  jbyte SendBuf[9];
	  //jbyteArray SendBuf = NULL;
	   // 新建一个字节数组
	 // SendBuf = (*env)->NewByteArray( env, 9 );
	  LOGI("GetCardInfo");
#if 1
	  SendBuf[0] =CMD_PACK_HEAD;
	  SendBuf[1] =0x00;
	  SendBuf[2] =0x04;
	  SendBuf[3] =(jbyte)CMD_RFCARD;
	  SendBuf[4] =0x31;
	  SendBuf[5] =(jbyte)((dWaitTimeouts)/256);
	  SendBuf[6] =(jbyte)((dWaitTimeouts)%256);
	  SendBuf[7] =(jbyte)Java_android_serialport_SerialPort_Checksum(SendBuf,6);
	  SendBuf[8] =CMD_PACK_END;
	  Java_android_serialport_SerialPort_WriteBuf(SendBuf,9);
      LOGI("write data len:%d,len:%d",SendBuf[0],SendBuf[7]);
	 // Java_android_serialport_SerialPort_WriteData(env,thiz,SendBuf);
#endif
	//return 1;
 }


#if 0
/*
 * 线程Run
 */
void* threadreadTtyData(void* arg){
	   if(!(arg)){
	       return NULL;
	    }

	    char *buf;
	    buf=(char *)malloc(200);
	    int result = 0,ret;
	    fd_set readfd;
	    struct timeval timeout;
	    while(mOpen){
	           timeout.tv_sec = 2;//设定超时秒数
	           timeout.tv_usec = 0;//设定超时毫秒数

	           FD_ZERO(&readfd);//清空集合
	           FD_SET(mTtyfd,&readfd);///* 把要检测的句柄mTtyfd加入到集合里 */
	           ret = select(mTtyfd+1,&readfd,NULL,NULL,&timeout);/* 检测我们上面设置到集合readfd里的句柄是否有可读信息 */
	           switch(ret){
	           case -1:/* 这说明select函数出错 */
	               result = -1;
	               //LOGE("mTtyfd read failure");
	               break;
	           case 0:/* 说明在我们设定的时间值5秒加0毫秒的时间内，mTty的状态没有发生变化 */
	               break;
	           default:/* 说明等待时间还未到5秒加0毫秒，mTty的状态发生了变化 */
	               if(FD_ISSET(mTtyfd,&readfd)){/* 先判断一下mTty这外被监视的句柄是否真的变成可读的了 */
	                  int len = read(mTtyfd,buf,sizeof(buf));
	                  /**发送数据**/
	                  //JNIMyObserver *l = static_cast<JNIMyObserver *>(arg);
	                  //l->OnEvent(buf,len,RECEIVE_DATA_INDEX);
	                  LOGI("threadreadTtyData Reivice");
	                  memset(buf,0,sizeof(buf));
	               }
	               break;
	           }
	           if(result == -1){
	               break;
	           }
	        }
	        if(buf != NULL){
	           //delete buf;
	           buf = NULL;
	        }
	        //LOGE("stop run!");
	        return NULL;
}


/*
 * _receiveMsgFromTty
 */
static void JNICALL Java_android_serialport_SerialPort_ReviceData(JNIEnv *env,jobject clazz){
    LOGI("com_uart_manager_TtyControl__receiveMsgFromTty");
    if(mTtyfd < 0){
       LOGE("mTtyfd open failure ,non't read");
       return ;
    }
    pthread_t id;
    int ret;
    ret = pthread_create(&id,NULL,threadreadTtyData,NULL);
    if(ret != 0){
       LOGE("create receiver thread failure ");
    }else{
       LOGW("create read data thred success");
 }

}



/*
	 函数功能:寻卡
	 返回值  :
	 0x00	连接成功
	 0x01	RFID-SIMÎ未连接
	 0x05	RFID-SIM连接失败
	 0x06	RFID-SIM等待卡进入感应区

  若返回00
       返回的数据=1个字节长度+卡号

*/

#endif




