// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_CONTINUE_WINDOW_H_
#define REMOTING_HOST_CONTINUE_WINDOW_H_

#include "base/basictypes.h"
#include "base/memory/weak_ptr.h"
#include "base/timer.h"
#include "remoting/host/host_window.h"
#include "remoting/host/ui_strings.h"

namespace remoting {

class ContinueWindow : public HostWindow {
 public:
  virtual ~ContinueWindow();

  // HostWindow override.
  virtual void Start(
      const base::WeakPtr<ClientSessionControl>& client_session_control)
      OVERRIDE;

  // Resumes paused client session.
  void ContinueSession();

  // Disconnects the client session.
  void DisconnectSession();

 protected:
  explicit ContinueWindow(const UiStrings& ui_strings);

  // Shows and hides the UI.
  virtual void ShowUi() = 0;
  virtual void HideUi() = 0;

  const UiStrings& ui_strings() const { return ui_strings_; }

 private:
  // Invoked periodically to ask for the local user whether the session should
  // be continued.
  void OnSessionExpired();

  // Used to disconnect the client session.
  base::WeakPtr<ClientSessionControl> client_session_control_;

  // Used to disconnect the client session when timeout expires.
  base::OneShotTimer<ContinueWindow> disconnect_timer_;

  // Used to ask the local user whether the session should be continued.
  base::OneShotTimer<ContinueWindow> session_expired_timer_;

  // Localized UI strings.
  UiStrings ui_strings_;

  DISALLOW_COPY_AND_ASSIGN(ContinueWindow);
};

}  // namespace remoting

#endif  // REMOTING_HOST_CONTINUE_WINDOW_H_
