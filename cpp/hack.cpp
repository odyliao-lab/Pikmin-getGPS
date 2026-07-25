#include "hack.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
#include <time.h>
#include <unistd.h>

#include "And64InlineHook.hpp"
#include "log.h"
#include "xdl.h"

namespace {

constexpr char kTargetVersion[] = "149.0";
constexpr int kTargetVersionCode = 1784082813;
// ExpeditionListItem.<Start>b__34_0(Unit), the actual button callback. The
// compiler duplicated OnClick into this lambda, so hooking OnClick itself
// does not observe taps.
constexpr uintptr_t kRvaListItemOnClick = 0x5E2E4F8;
constexpr uintptr_t kRvaGetSpawnLocation = 0x5E519EC;
constexpr uintptr_t kListItemCurrentTaskOffset = 0xD8;

constexpr uint8_t kListItemOnClickSignature[] = {
    0xFF, 0xC3, 0x00, 0xD1, 0xFE, 0x57, 0x01, 0xA9,
    0xF4, 0x4F, 0x02, 0xA9, 0x74, 0x8F, 0x04, 0xB0
};

struct LatLng {
    double lat;
    double lng;
};

using ListItemOnClickFn = void (*)(void *self, uint8_t unit, void *method);
using GetSpawnLocationFn = LatLng (*)(void *expedition, void *method);

JavaVM *g_vm = nullptr;
ListItemOnClickFn g_original_on_click = nullptr;
GetSpawnLocationFn g_get_spawn_location = nullptr;
std::mutex g_copy_mutex;
double g_last_lat = 999.0;
double g_last_lng = 999.0;
int64_t g_last_copy_ms = 0;

int64_t monotonic_ms() {
    timespec now{};
    clock_gettime(CLOCK_MONOTONIC, &now);
    return static_cast<int64_t>(now.tv_sec) * 1000 + now.tv_nsec / 1000000;
}

bool valid_coordinate(const LatLng &point) {
    return std::isfinite(point.lat) && std::isfinite(point.lng) &&
           point.lat >= -90.0 && point.lat <= 90.0 &&
           point.lng >= -180.0 && point.lng <= 180.0 &&
           (std::fabs(point.lat) > 0.0000001 || std::fabs(point.lng) > 0.0000001);
}

bool clear_jni_exception(JNIEnv *env, const char *stage) {
    if (!env->ExceptionCheck()) return false;
    LOGE("JNI exception at %s", stage);
    env->ExceptionClear();
    return true;
}

bool copy_to_clipboard_and_toast(const char *coordinates) {
    if (g_vm == nullptr) return false;

    JNIEnv *env = nullptr;
    bool attached_here = false;
    jint status = g_vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
    if (status == JNI_EDETACHED) {
        if (g_vm->AttachCurrentThread(&env, nullptr) != JNI_OK) {
            LOGE("AttachCurrentThread failed");
            return false;
        }
        attached_here = true;
    } else if (status != JNI_OK || env == nullptr) {
        LOGE("GetEnv failed: %d", status);
        return false;
    }

    bool copied = false;
    if (env->PushLocalFrame(32) != JNI_OK) {
        if (attached_here) g_vm->DetachCurrentThread();
        return false;
    }

    jclass activity_thread = env->FindClass("android/app/ActivityThread");
    jmethodID current_application = activity_thread
        ? env->GetStaticMethodID(activity_thread, "currentApplication",
                                 "()Landroid/app/Application;")
        : nullptr;
    jobject application = current_application
        ? env->CallStaticObjectMethod(activity_thread, current_application)
        : nullptr;
    if (clear_jni_exception(env, "currentApplication") || application == nullptr) {
        goto cleanup;
    }

    {
        jclass context_class = env->FindClass("android/content/Context");
        jmethodID get_system_service = context_class
            ? env->GetMethodID(context_class, "getSystemService",
                               "(Ljava/lang/String;)Ljava/lang/Object;")
            : nullptr;
        jstring clipboard_name = env->NewStringUTF("clipboard");
        jobject clipboard = get_system_service
            ? env->CallObjectMethod(application, get_system_service, clipboard_name)
            : nullptr;
        if (clear_jni_exception(env, "getSystemService") || clipboard == nullptr) {
            goto cleanup;
        }

        jclass clip_data_class = env->FindClass("android/content/ClipData");
        jmethodID new_plain_text = clip_data_class
            ? env->GetStaticMethodID(
                  clip_data_class, "newPlainText",
                  "(Ljava/lang/CharSequence;Ljava/lang/CharSequence;)"
                  "Landroid/content/ClipData;")
            : nullptr;
        jstring label = env->NewStringUTF("Pikmin GPS");
        jstring text = env->NewStringUTF(coordinates);
        jobject clip = new_plain_text
            ? env->CallStaticObjectMethod(clip_data_class, new_plain_text, label, text)
            : nullptr;
        if (clear_jni_exception(env, "ClipData.newPlainText") || clip == nullptr) {
            goto cleanup;
        }

        jclass clipboard_class = env->GetObjectClass(clipboard);
        jmethodID set_primary_clip = clipboard_class
            ? env->GetMethodID(clipboard_class, "setPrimaryClip",
                               "(Landroid/content/ClipData;)V")
            : nullptr;
        if (set_primary_clip == nullptr) goto cleanup;
        env->CallVoidMethod(clipboard, set_primary_clip, clip);
        if (clear_jni_exception(env, "ClipboardManager.setPrimaryClip")) {
            goto cleanup;
        }
        copied = true;

        // The debounce worker is attached to the VM but does not have a
        // Looper by default. Toast creates a Handler internally, so prepare a
        // thread Looper before constructing it.
        jclass looper_class = env->FindClass("android/os/Looper");
        jmethodID my_looper = looper_class
            ? env->GetStaticMethodID(looper_class, "myLooper",
                                     "()Landroid/os/Looper;")
            : nullptr;
        jobject looper = my_looper
            ? env->CallStaticObjectMethod(looper_class, my_looper)
            : nullptr;
        if (!clear_jni_exception(env, "Looper.myLooper") &&
            looper == nullptr && looper_class != nullptr) {
            jmethodID prepare = env->GetStaticMethodID(
                looper_class, "prepare", "()V");
            if (prepare != nullptr) {
                env->CallStaticVoidMethod(looper_class, prepare);
                clear_jni_exception(env, "Looper.prepare");
            }
        }

        jclass toast_class = env->FindClass("android/widget/Toast");
        jmethodID make_text = toast_class
            ? env->GetStaticMethodID(
                  toast_class, "makeText",
                  "(Landroid/content/Context;Ljava/lang/CharSequence;I)"
                  "Landroid/widget/Toast;")
            : nullptr;
        jstring toast_text = env->NewStringUTF("GPS 已複製");
        jobject toast = make_text
            ? env->CallStaticObjectMethod(toast_class, make_text, application, toast_text, 0)
            : nullptr;
        if (!clear_jni_exception(env, "Toast.makeText") && toast != nullptr) {
            jmethodID show = env->GetMethodID(toast_class, "show", "()V");
            if (show != nullptr) {
                env->CallVoidMethod(toast, show);
                clear_jni_exception(env, "Toast.show");
            }
        }
    }

cleanup:
    env->PopLocalFrame(nullptr);
    if (attached_here) g_vm->DetachCurrentThread();
    return copied;
}

void copy_expedition_gps(void *expedition) {
    if (expedition == nullptr || g_get_spawn_location == nullptr) return;

    LatLng point = g_get_spawn_location(expedition, nullptr);
    if (!valid_coordinate(point)) {
        LOGW("selected expedition returned invalid GPS: %.7f,%.7f", point.lat, point.lng);
        return;
    }

    const int64_t now = monotonic_ms();
    {
        std::lock_guard<std::mutex> lock(g_copy_mutex);
        if (std::fabs(point.lat - g_last_lat) < 0.00000001 &&
            std::fabs(point.lng - g_last_lng) < 0.00000001 &&
            now - g_last_copy_ms < 1200) {
            return;
        }
        g_last_lat = point.lat;
        g_last_lng = point.lng;
        g_last_copy_ms = now;
    }

    char coordinates[64];
    snprintf(coordinates, sizeof(coordinates), "%.7f,%.7f", point.lat, point.lng);
    if (copy_to_clipboard_and_toast(coordinates)) {
        LOGI("GPS copied: %s", coordinates);
    } else {
        LOGE("failed to copy GPS: %s", coordinates);
    }
}

void hooked_list_item_on_click(void *self, uint8_t unit, void *method) {
    if (self != nullptr) {
        auto expedition = *reinterpret_cast<void **>(
            reinterpret_cast<uintptr_t>(self) + kListItemCurrentTaskOffset);
        copy_expedition_gps(expedition);
    }
    if (g_original_on_click != nullptr) {
        g_original_on_click(self, unit, method);
    }
}

bool install_hook() {
    void *handle = xdl_open("libil2cpp.so", XDL_DEFAULT);
    if (handle == nullptr) return false;

    xdl_info_t info{};
    if (xdl_info(handle, XDL_DI_DLINFO, &info) != 0 || info.dli_fbase == nullptr) {
        LOGE("unable to resolve libil2cpp base");
        return true;
    }

    auto base = reinterpret_cast<uintptr_t>(info.dli_fbase);
    auto *click_target =
        reinterpret_cast<void *>(base + kRvaListItemOnClick);
    if (memcmp(click_target, kListItemOnClickSignature,
               sizeof(kListItemOnClickSignature)) != 0) {
        LOGE("signature mismatch; expected Pikmin %s (%d), refusing hook",
             kTargetVersion, kTargetVersionCode);
        return true;
    }

    g_get_spawn_location = reinterpret_cast<GetSpawnLocationFn>(
        base + kRvaGetSpawnLocation);
    A64HookFunction(click_target,
                    reinterpret_cast<void *>(hooked_list_item_on_click),
                    reinterpret_cast<void **>(&g_original_on_click));
    if (g_original_on_click == nullptr) {
        LOGE("hook installation failed");
    } else {
        LOGI("list-item click hook installed for Pikmin %s (%d): target=%p",
             kTargetVersion, kTargetVersionCode, click_target);
    }
    return true;
}

}  // namespace

void hack_prepare(const char *game_data_dir, JavaVM *vm) {
    (void)game_data_dir;
    g_vm = vm;
    for (int attempt = 0; attempt < 120; ++attempt) {
        if (install_hook()) return;
        sleep(1);
    }
    LOGE("libil2cpp.so did not load within 120 seconds");
}
