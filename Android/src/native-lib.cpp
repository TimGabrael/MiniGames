#include <jni.h>
#include <string>
#include <android/log.h>
#include <android/sync.h>
#include "CommonCollection.h"
#include "Network/Networking.h"
#include "logging.h"
#include <future>


JavaVM* jvm;
TCPSocket sock;

std::future<void> connectFut;
void ConnectToServer()
{
    JNIEnv* env;
    jvm->AttachCurrentThread(&env, NULL);

    NetError errCode = sock.Connect(DEBUG_IP, DEBUG_PORT);
    if (errCode != NetError::OK)
    {
        LOG(" FAILED TO CONNECT %d", (int)errCode);

    }
    else {
        TestPacket test("Moin von Android");
        sock.SendPacket(&test);
    }
    jvm->DetachCurrentThread();
}

extern "C" JNIEXPORT void JNICALL
Java_com_GabelStudio_MiniGames_MainActivity_Initialize(
    JNIEnv * env,
    jobject activity) {

    static bool just_once = false;
    if (!just_once)
    {
        env->GetJavaVM(&jvm);

        connectFut = std::async(ConnectToServer);

        just_once = true;
    }

}

std::future<void> voidFut;
void doAsyncTask(jobject handler)
{
    JNIEnv* env = nullptr;
    jvm->AttachCurrentThread(&env, NULL);
    jclass clazz = env->GetObjectClass(handler);
    jmethodID funcID = env->GetMethodID(clazz, "TestFunc", "(I)V");
    env->CallVoidMethod(handler, funcID, 15);


    env->DeleteGlobalRef(handler);

    jvm->DetachCurrentThread();
}

extern "C" JNIEXPORT void JNICALL
Java_com_GabelStudio_MiniGames_MainActivity_MessageServer(JNIEnv * env, jobject /*this*/, jobject handler) {
    //TestPacket press("Button Habe ich gedr�ckt JUHU");
    //sock.SendPacket(&press);


    //jclass clazz = env->FindClass("com/GabelStudio/MiniGames/MainActivity");
    //jmethodID funcID = env->GetStaticMethodID(clazz, "TestFunc", "()V");
    //env->CallStaticVoidMethod(clazz, funcID);

    //jclass clazz = env->GetObjectClass(handler);
    //jmethodID funcID = env->GetMethodID(clazz, "TestFunc", "()V");
    //env->CallVoidMethod(handler, funcID);

    jobject newObject = env->NewGlobalRef(handler);

    voidFut = std::async(doAsyncTask, newObject);


}