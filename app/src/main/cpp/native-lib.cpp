#include <jni.h>
#include <string>

extern "C" void test_all(char *s, size_t n) {
#ifdef __aarch64__
    strncpy(s, "Hello, aarch64!\n", n);
#else
    strncpy(s, "Hello, world!\n", n);
#endif
}

extern "C"
JNIEXPORT jstring JNICALL
Java_barrbrain_simdtools_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    char buffer[2048] = { 0 };
    test_all(buffer, sizeof buffer);
    return env->NewStringUTF(buffer);
}
