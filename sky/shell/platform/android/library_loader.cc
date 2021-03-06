// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/base_jni_onload.h"
#include "base/android/base_jni_registrar.h"
#include "base/android/jni_android.h"
#include "base/android/jni_registrar.h"
#include "base/android/library_loader/library_loader_hooks.h"
#include "base/bind.h"
#include "base/logging.h"
#include "mojo/android/system/base_run_loop.h"
#include "mojo/android/system/core_impl.h"
#include "sky/engine/bindings/jni/dart_jni.h"
#include "sky/shell/platform/android/platform_view_android.h"
#include "sky/shell/platform/android/sky_main.h"
#include "sky/shell/platform/android/tracing_controller.h"
#include "sky/shell/platform/android/update_service_android.h"

namespace {

base::android::RegistrationMethod kSkyRegisteredMethods[] = {
    {"CoreImpl", mojo::android::RegisterCoreImpl},
    {"BaseRunLoop", mojo::android::RegisterBaseRunLoop},
    {"PlatformViewAndroid", sky::shell::PlatformViewAndroid::Register},
    {"SkyMain", sky::shell::RegisterSkyMain},
    {"TracingController", sky::shell::RegisterTracingController},
    {"UpdateService", sky::shell::RegisterUpdateService},
};

bool RegisterJNI(JNIEnv* env) {
  if (!base::android::RegisterJni(env))
    return false;

  return RegisterNativeMethods(env, kSkyRegisteredMethods,
                               arraysize(kSkyRegisteredMethods));
}

bool InitJNI() {
  return blink::DartJni::InitJni();
}

}  // namespace

// This is called by the VM when the shared library is first loaded.
JNI_EXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  std::vector<base::android::RegisterCallback> register_callbacks;
  register_callbacks.push_back(base::Bind(&RegisterJNI));

  std::vector<base::android::InitCallback> init_callbacks;
  init_callbacks.push_back(base::Bind(&InitJNI));

  if (!base::android::OnJNIOnLoadRegisterJNI(vm, register_callbacks) ||
      !base::android::OnJNIOnLoadInit(init_callbacks)) {
    return -1;
  }

  return JNI_VERSION_1_4;
}
