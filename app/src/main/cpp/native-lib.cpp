#include <jni.h>
#include <string>

extern "C" int main(void);

extern "C"
JNIEXPORT jstring JNICALL
Java_barrbrain_simdtools_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    freopen("/sdcard/simd_tools", "w", stdout);
    main();
    fflush(stdout);
    return env->NewStringUTF(hello.c_str());
}
