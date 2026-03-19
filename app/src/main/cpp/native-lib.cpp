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

std::mutex pinMutex;
std::condition_variable pinCv;
std::string receivedPin;
bool pinReady = false;



mbedtls_entropy_context entropy;
mbedtls_ctr_drbg_context ctr_drbg;
const char *personalization = "andro25-sample-app";

JavaVM* gJvm = nullptr;

JNIEXPORT jint JNICALL JNI_OnLoad (JavaVM* pjvm, void* reserved)
{
    gJvm = pjvm;
    return JNI_VERSION_1_6;
}

JNIEnv* getEnv (bool& detach)
{
    JNIEnv* env = nullptr;
    int status = gJvm->GetEnv ((void**)&env, JNI_VERSION_1_6);
    detach = false;
    if (status == JNI_EDETACHED) {
        if (gJvm->AttachCurrentThread(&env, nullptr) != 0)
            return nullptr;
        detach = true;
    }
    return env;
}

void releaseEnv (bool detach, JNIEnv* env)
{
    if (detach && (gJvm != nullptr))
    {
        gJvm->DetachCurrentThread ();
    }
}


extern "C" JNIEXPORT jstring JNICALL
Java_com_example_andro25_MainActivity_stringFromJNI(
        JNIEnv* env,
        jclass /* this */) {
    SLOG_PATTERN("%v");
    LOG_INFO("Hello from c++ %d", 2026);
    SLOG_INFO("Hello from spdlog {0}", 2026);
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_andro25_MainActivity_initRng(JNIEnv *env, jclass clazz) {
    mbedtls_entropy_init( &entropy );
    mbedtls_ctr_drbg_init( &ctr_drbg );

    return mbedtls_ctr_drbg_seed( &ctr_drbg , mbedtls_entropy_func, &entropy,
                                  (const unsigned char *) personalization,
                                  strlen( personalization ) );
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_example_andro25_MainActivity_randomBytes(JNIEnv *env, jclass jclass, jint no) {
    uint8_t* buf = new uint8_t [no];
    mbedtls_ctr_drbg_random(&ctr_drbg, buf, no);
    jbyteArray rnd = env->NewByteArray(no);
    env->SetByteArrayRegion(rnd, 0, no, (jbyte *)buf);
    delete[] buf;
    return rnd;
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_example_andro25_MainActivity_encrypt(JNIEnv *env, jclass jclass, jbyteArray key, jbyteArray data)
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
Java_com_example_andro25_MainActivity_decrypt(JNIEnv *env, jclass jclass, jbyteArray key, jbyteArray data)
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

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_example_andro25_MainActivity_transaction(JNIEnv *xenv, jobject xthiz, jbyteArray xtrd) {
    jobject thiz = xenv->NewGlobalRef(xthiz);
    jbyteArray trd  = (jbyteArray)xenv->NewGlobalRef(xtrd);

    std::thread ([thiz, trd]() {
        bool detach = false;
        JNIEnv *env = getEnv(detach);

        jclass cls = env->GetObjectClass(thiz);

        jmethodID enterPinId = env->GetMethodID(
                cls, "enterPin", "(ILjava/lang/String;)V");

        jmethodID resultId = env->GetMethodID(cls, "transactionResult", "(Z)V");

        uint8_t *p = (uint8_t *) env->GetByteArrayElements(trd, 0);
        jsize sz = env->GetArrayLength(trd);

        if ((sz != 9) || (p[0] != 0x9F) || (p[1] != 0x02) || (p[2] != 0x06)){
            env->CallVoidMethod(thiz, resultId, false);
        } else {
            char buf[13];

            for (int i = 0; i < 6; i++) {
                uint8_t n = *(p + 3 + i);
                buf[i * 2] = ((n & 0xF0) >> 4) + '0';
                buf[i * 2 + 1] = (n & 0x0F) + '0';
            }

            buf[12] = 0x00;

            jstring jamount = (jstring) env->NewStringUTF(buf);
            int ptc = 3;
            bool ok = false;

            while (ptc > 0) {

                {
                    std::lock_guard<std::mutex> lock(pinMutex);
                    pinReady = false;
                }

                env->CallVoidMethod(thiz, enterPinId, ptc, jamount);

                std::unique_lock<std::mutex> lock(pinMutex);
                pinCv.wait(lock, [] { return pinReady; });

                if (receivedPin == "1234") {
                    ok = true;
                    break;
                }

                ptc--;
            }

            env->CallVoidMethod(thiz, resultId, ok);
        }

        env->ReleaseByteArrayElements(trd, (jbyte *)p, JNI_ABORT);
        env->DeleteLocalRef(cls);
        env->DeleteGlobalRef(thiz);
        env->DeleteGlobalRef(trd);
        releaseEnv(detach, env);
        SLOG_INFO("thread detached successfully");
    }).detach();


    return true;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_andro25_MainActivity_providePin(JNIEnv *env, jobject thiz, jstring jpin){
    if (!jpin) return;
    const char* utf = env->GetStringUTFChars(jpin, nullptr);
    if (!utf) return;

    {
        std::lock_guard<std::mutex> lock(pinMutex);
        receivedPin = utf;
        pinReady = true;
    }

    env->ReleaseStringUTFChars(jpin, utf);
    pinCv.notify_one();
}