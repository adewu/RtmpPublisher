#include <stdio.h>
#include <jni.h>
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

AVFormatContext *g_fmt_ctx;
AVStream *g_stream;
//视音频流对应的结构体，用于视音频编解码。
AVCodecContext *g_codec_ctx;
AVCodec *g_codec;
AVPacket g_packet; // 存储压缩数据（视频对应H.264等码流数据，音频对应AAC/MP3等码流数据）
AVFrame *g_frame; // 存储非压缩的数据（视频对应RGB/YUV像素数据，音频对应PCM采样数据）

int g_frame_count = 0;
int g_yuv_width;
int g_yuv_height;
int g_y_length;
int g_uv_length;
int64_t g_start_time;


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


JNIEXPORT jint JNICALL
Java_github_qq2653203_rtmppublisher_MainActivity_initFFmpeg(JNIEnv *env, jobject thiz,
                                                            jobject buffer, jint width, jint height,
                                                            jstring url) {

    int ret;

    const char *PATH = (*env)->GetStringUTFChars(env, url, 0);

    g_yuv_width = width;
    g_yuv_height = height;
    g_y_length = width * height;
    g_uv_length = width * height / 4;

    av_log_set_callback(custom_log);

    av_register_all();

    ret = avformat_network_init();

    /**
     * output initialize
     */
    ret = avformat_alloc_output_context2(&g_fmt_ctx,NULL,"flv",PATH);

    /**
     * 初始化编码器
     * 函数的参数是一个编码器的ID，返回查找到的编码器（没有找到就返回NULL）
     */
    g_codec = avcodec_find_encoder(AV_CODEC_ID_H264);

    if (!g_codec){
        LOGE("can not find encoder!\n");
        return -1;
    }

    g_codec_ctx = avcodec_alloc_context3(g_codec);
    g_codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    g_codec_ctx->width = width;
    g_codec_ctx->height = width;
    g_codec_ctx->time_base.num = 1;
    g_codec_ctx->time_base.den = 25;
    g_codec_ctx->bit_rate = 400000;
    g_codec_ctx->gop_size = 250;

    /**
     * 有些格式的流的headers需要分开
     */
    if (g_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        g_codec_ctx->flags |= CODEC_FLAG_GLOBAL_HEADER;
}