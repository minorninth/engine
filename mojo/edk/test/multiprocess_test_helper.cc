// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/test/multiprocess_test_helper.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "base/test/test_timeouts.h"
#include "mojo/edk/embedder/platform_channel_pair.h"

using mojo::platform::ScopedPlatformHandle;

namespace mojo {
namespace test {

const char kPlatformChannelHandleInfoSwitch[] = "platform-channel-handle-info";

MultiprocessTestHelper::MultiprocessTestHelper()
    : platform_channel_pair_(new embedder::PlatformChannelPair()) {
  server_platform_handle = platform_channel_pair_->PassServerHandle();
}

MultiprocessTestHelper::~MultiprocessTestHelper() {
  CHECK(!test_child_.IsValid());
  server_platform_handle.reset();
  platform_channel_pair_.reset();
}

void MultiprocessTestHelper::StartChild(const std::string& test_child_name) {
  StartChildWithExtraSwitch(test_child_name, std::string(), std::string());
}

void MultiprocessTestHelper::StartChildWithExtraSwitch(
    const std::string& test_child_name,
    const std::string& switch_string,
    const std::string& switch_value) {
  CHECK(platform_channel_pair_);
  CHECK(!test_child_name.empty());
  CHECK(!test_child_.IsValid());

  std::string test_child_main = test_child_name + "TestChildMain";

  std::string string_for_child;
  embedder::HandlePassingInformation handle_passing_info;
  platform_channel_pair_->PrepareToPassClientHandleToChildProcess(
      &string_for_child, &handle_passing_info);

  base::CommandLine command_line(
      base::GetMultiProcessTestChildBaseCommandLine());
  command_line.AppendSwitchASCII(kPlatformChannelHandleInfoSwitch,
                                 string_for_child);

  if (!switch_string.empty()) {
    CHECK(!command_line.HasSwitch(switch_string));
    if (!switch_value.empty())
      command_line.AppendSwitchASCII(switch_string, switch_value);
    else
      command_line.AppendSwitch(switch_string);
  }

  base::LaunchOptions options;
  options.fds_to_remap = &handle_passing_info;

  test_child_ =
      base::SpawnMultiProcessTestChild(test_child_main, command_line, options);
  platform_channel_pair_->ChildProcessLaunched();

  CHECK(test_child_.IsValid());
}

int MultiprocessTestHelper::WaitForChildShutdown() {
  CHECK(test_child_.IsValid());

  int rv = -1;
  CHECK(
      test_child_.WaitForExitWithTimeout(TestTimeouts::action_timeout(), &rv));
  test_child_.Close();
  return rv;
}

bool MultiprocessTestHelper::WaitForChildTestShutdown() {
  return WaitForChildShutdown() == 0;
}

// static
void MultiprocessTestHelper::ChildSetup() {
  CHECK(base::CommandLine::InitializedForCurrentProcess());
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  client_platform_handle =
      embedder::PlatformChannelPair::PassClientHandleFromParentProcess(
          command_line.GetSwitchValueASCII(kPlatformChannelHandleInfoSwitch));
  CHECK(client_platform_handle.is_valid());
}

// static
ScopedPlatformHandle MultiprocessTestHelper::client_platform_handle;

}  // namespace test
}  // namespace mojo
