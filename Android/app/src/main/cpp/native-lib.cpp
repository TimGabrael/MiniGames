#include <jni.h>
#include <string>
#include "CommonCollection.h"
#include <android/log.h>


TCPSocket sock;
extern "C" JNIEXPORT jstring JNICALL
Java_com_GabelStudio_MiniGames_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C HAHA";
    NetError errCode = sock.Connect(DEBUG_IP, DEBUG_PORT);

    __android_log_print(ANDROID_LOG_ERROR, "TTT", "%d", (int)errCode);

    TestPacket test("Moin von Android");
    sock.SendPacket(&test);



    return env->NewStringUTF(hello.c_str());
}