#include <jni.h>
#include <string>

extern "C" void test_subtract_average_all(char *, size_t);

extern "C"
JNIEXPORT jstring JNICALL
Java_barrbrain_simdtools_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    char buffer[2048] = { 0 };
    test_subtract_average_all(buffer, sizeof buffer);
    return env->NewStringUTF(buffer);
}
