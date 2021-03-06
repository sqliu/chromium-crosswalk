// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_AUDIO_NODE_H_
#define CHROMEOS_DBUS_AUDIO_NODE_H_

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "chromeos/chromeos_export.h"

namespace chromeos {

struct CHROMEOS_EXPORT AudioNode {
  bool is_input;
  uint64 id;
  std::string device_name;
  std::string type;
  std::string name;
  bool active;

  AudioNode();
  std::string ToString() const;
};

typedef std::vector<AudioNode> AudioNodeList;

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_AUDIO_NODE_H_
