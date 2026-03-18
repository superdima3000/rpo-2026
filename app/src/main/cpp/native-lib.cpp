#include <jni.h>
#include <string>
#include <android/log.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/android_sink.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/des.h>

#define LOG_INFO(...) __android_log_print(ANDROID_LOG_INFO, "andro25_ndk", __VA_ARGS__)
#define SLOG_INFO(...) android_logger->info( __VA_ARGS__ )
#define SLOG_PATTERN(...) android_logger->set_pattern(__VA_ARGS__)

auto android_logger = spdlog::android_logger_mt("android", "andro25_ndk");
mbedtls_entropy_context entropy;
mbedtls_ctr_drbg_context ctr_drbg;
char *personalization = "andro25-sample-app";


extern "C" JNIEXPORT jstring JNICALL
Java_com_example_andro25_nativelibs_NativeLib_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    SLOG_PATTERN("%v");
    LOG_INFO("Hello from c++ %d", 2026);
    SLOG_INFO("Hello from spdlog {0}", 2026);
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_andro25_nativelibs_NativeLib_initRng(JNIEnv *env, jobject clazz) {
    mbedtls_entropy_init( &entropy );
    mbedtls_ctr_drbg_init( &ctr_drbg );

    return mbedtls_ctr_drbg_seed( &ctr_drbg , mbedtls_entropy_func, &entropy,
                                  (const unsigned char *) personalization,
                                  strlen( personalization ) );
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_example_andro25_nativelibs_NativeLib_randomBytes(JNIEnv *env, jobject jclass, jint no) {
    uint8_t* buf = new uint8_t [no];
    mbedtls_ctr_drbg_random(&ctr_drbg, buf, no);
    jbyteArray rnd = env->NewByteArray(no);
    env->SetByteArrayRegion(rnd, 0, no, (jbyte *)buf);
    delete[] buf;
    return rnd;
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_example_andro25_nativelibs_NativeLib_encrypt(JNIEnv *env, jobject jclass, jbyteArray key, jbyteArray data)
{
    jsize ksz = env->GetArrayLength(key);
    jsize dsz = env->GetArrayLength(data);
    if ((ksz != 16) || (dsz <= 0)) {
        return env->NewByteArray(0);
    }
    mbedtls_des3_context ctx;
    mbedtls_des3_init(&ctx);

    jbyte * pkey = env->GetByteArrayElements(key, 0);

    // Паддинг PKCS#5
    int rst = dsz % 8;
    int pad = 8 - rst;
    if (pad == 0) pad = 8;
    int sz = dsz + pad;
    uint8_t * buf = new uint8_t[sz];
    for (int i = 0; i < pad; i++)
        buf[dsz + i] = pad;
    jbyte * pdata = env->GetByteArrayElements(data, 0);
    std::copy(pdata, pdata + dsz, buf);
    mbedtls_des3_set2key_enc(&ctx, (uint8_t *)pkey);
    int cn = sz / 8;
    for (int i = 0; i < cn; i++)
        mbedtls_des3_crypt_ecb(&ctx, buf + i*8, buf + i*8);
    jbyteArray dout = env->NewByteArray(sz);
    env->SetByteArrayRegion(dout, 0, sz, (jbyte *)buf);
    delete[] buf;
    env->ReleaseByteArrayElements(key, pkey, 0);
    env->ReleaseByteArrayElements(data, pdata, 0);
    return dout;
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_example_andro25_nativelibs_NativeLib_decrypt(JNIEnv *env, jobject jclass, jbyteArray key, jbyteArray data)
{
    jsize ksz = env->GetArrayLength(key);
    jsize dsz = env->GetArrayLength(data);
    if ((ksz != 16) || (dsz <= 0) || ((dsz % 8) != 0)) {
        return env->NewByteArray(0);
    }
    mbedtls_des3_context ctx;
    mbedtls_des3_init(&ctx);

    jbyte * pkey = env->GetByteArrayElements(key, 0);

    uint8_t * buf = new uint8_t[dsz];

    jbyte * pdata = env->GetByteArrayElements(data, 0);
    std::copy(pdata, pdata + dsz, buf);
    mbedtls_des3_set2key_dec(&ctx, (uint8_t *)pkey);
    int cn = dsz / 8;
    for (int i = 0; i < cn; i++)
        mbedtls_des3_crypt_ecb(&ctx, buf + i*8, buf +i*8);

    //PKCS#5. упрощено. по соображениям безопасности надо проверить каждый байт паддинга
    int pad = buf[dsz-1];
    int sz = dsz - pad;

    //проверка по длине
    if (pad < 1 || pad > 8) {
        return env->NewByteArray(0);
    }

    //проверка по заполнению
    for (int i = 0; i < pad; i++) {
        if (buf[dsz - 1 - i] != pad) {
            return env->NewByteArray(0);
        }
    }

    jbyteArray dout = env->NewByteArray(sz);
    env->SetByteArrayRegion(dout, 0, sz, (jbyte *)buf);
    delete[] buf;
    env->ReleaseByteArrayElements(key, pkey, 0);
    env->ReleaseByteArrayElements(data, pdata, 0);
    return dout;
}