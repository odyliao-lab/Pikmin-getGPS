#include <cstring>
#include <thread>

#include "game.h"
#include "hack.h"
#include "log.h"
#include "zygisk.hpp"

using zygisk::Api;
using zygisk::AppSpecializeArgs;

class PikminGpsCopyModule final : public zygisk::ModuleBase {
public:
    void onLoad(Api *loaded_api, JNIEnv *loaded_env) override {
        api = loaded_api;
        env = loaded_env;
        env->GetJavaVM(&vm);
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        const char *package_name = env->GetStringUTFChars(args->nice_name, nullptr);
        const char *app_data_dir = env->GetStringUTFChars(args->app_data_dir, nullptr);

        if (strcmp(package_name, GamePackageName) == 0) {
            enabled = true;
            game_data_dir = new char[strlen(app_data_dir) + 1];
            strcpy(game_data_dir, app_data_dir);
            LOGI("target package detected: %s", package_name);
        } else {
            api->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY);
        }

        env->ReleaseStringUTFChars(args->nice_name, package_name);
        env->ReleaseStringUTFChars(args->app_data_dir, app_data_dir);
    }

    void postAppSpecialize(const AppSpecializeArgs *) override {
        if (!enabled || vm == nullptr || game_data_dir == nullptr) return;
        std::thread worker(hack_prepare, game_data_dir, vm);
        worker.detach();
    }

private:
    Api *api = nullptr;
    JNIEnv *env = nullptr;
    JavaVM *vm = nullptr;
    bool enabled = false;
    char *game_data_dir = nullptr;
};

REGISTER_ZYGISK_MODULE(PikminGpsCopyModule)
