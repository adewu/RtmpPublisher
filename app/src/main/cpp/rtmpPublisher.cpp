#include <jni.h>
#include <string>
#include <time.h>
#include <assert.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavfilter/avfilter.h>
#include <libswscale/swscale.h>
#include <libavutil/log.h>
#include <libavutil/error.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>

#ifdef ANDROID

#include <android/log.h>

#define LOGE(format, ...)  __android_log_print(ANDROID_LOG_ERROR, "(-_-!)", format, ##__VA_ARGS__)
#define LOGI(format, ...)  __android_log_print(ANDROID_LOG_INFO,  "(^_^)", format, ##__VA_ARGS__)
#else
#define LOGE(format, ...)  printf("(-_-!) " format "\n", ##__VA_ARGS__)
#define LOGI(format, ...)  printf("(^_^) " format "\n", ##__VA_ARGS__)
#endif


AVFormatContext *ofmt_ctx;
AVStream *video_st;
//视音频流对应的结构体，用于视音频编解码。
AVCodecContext *pCodecCtx;
AVCodec *pCodec;
AVPacket enc_pkt; // 存储压缩数据（视频对应H.264等码流数据，音频对应AAC/MP3等码流数据）
AVFrame *pFrameYUV; // 存储非压缩的数据（视频对应RGB/YUV像素数据，音频对应PCM采样数据）

int framecnt = 0;
int yuv_width;
int yuv_height;
int y_length;
int uv_length;
int64_t start_time;

//Output FFmpeg's av_log()
void custom_log(void *ptr, int level, const char *fmt, va_list vl) {
    FILE *fp = fopen("/storage/emulated/0/av_log.txt", "a+");
    if (fp) {
        vfprintf(fp, fmt, vl);
        fflush(fp);
        fclose(fp);
    }
}

void print_error(int err) {
    char errbuf[128];
    const char *errbuf_ptr = errbuf;
    if (av_strerror(err, errbuf, sizeof(errbuf)) < 0)
        errbuf_ptr = strerror(AVUNERROR(err));
//    av_log(NULL, AV_LOG_ERROR, "%s: %s\n", "hehe", errbuf_ptr);
    LOGE(" %s", errbuf_ptr);
}

using namespace std;
extern "C"
JNIEXPORT void JNICALL
Java_github_qq2653203_rtmppublisher_MainActivity_initFFmpeg(JNIEnv *env,jobject thiz,jobject buffer,jint width, jint height,jstring url){

    char* pBuffer = (char*)env->GetDirectBufferAddress(buffer);
    if(pBuffer == nullptr){
        return ;
    }
}