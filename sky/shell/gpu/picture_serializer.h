// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SKY_SHELL_GPU_PICTURE_SERIALIZER_H_
#define SKY_SHELL_GPU_PICTURE_SERIALIZER_H_

#include "base/files/file_path.h"
#include "third_party/skia/include/core/SkPicture.h"

namespace sky {

void SerializePicture(const base::FilePath& file_name, SkPicture*);

}  // namespace sky

#endif  // SKY_SHELL_GPU_PICTURE_SERIALIZER_H_
