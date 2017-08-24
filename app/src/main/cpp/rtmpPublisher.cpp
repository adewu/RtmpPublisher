#include <jni.h>
#include <string>

using namespace std;
extern "C"
JNIEXPORT void JNICALL
Java_github_qq2653203_rtmppublisher_MainActivity_callMeMaybe(JNIEnv *env,jobject thiz,jobject buffer,jint length){

    char* pBuffer = (char*)env->GetDirectBufferAddress(buffer);
    if(pBuffer == NULL){
        return ;
    }
}