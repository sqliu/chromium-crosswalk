// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/variations/resource_request_allowed_notifier.h"

#include "base/command_line.h"
#include "chrome/common/chrome_switches.h"

ResourceRequestAllowedNotifier::ResourceRequestAllowedNotifier()
    : observer_requested_permission_(false),
      waiting_for_network_(false),
      waiting_for_user_to_accept_eula_(false),
      observer_(NULL) {
}

ResourceRequestAllowedNotifier::~ResourceRequestAllowedNotifier() {
  if (observer_)
    net::NetworkChangeNotifier::RemoveConnectionTypeObserver(this);
}

void ResourceRequestAllowedNotifier::Init(Observer* observer) {
  DCHECK(!observer_ && observer);
  observer_ = observer;

  net::NetworkChangeNotifier::AddConnectionTypeObserver(this);

  // Check this state during initialization. It is not expected to change until
  // the corresponding notification is received.
  waiting_for_network_ = net::NetworkChangeNotifier::IsOffline();

  eula_notifier_.reset(CreateEulaNotifier());
  if (eula_notifier_) {
    eula_notifier_->Init(this);
    waiting_for_user_to_accept_eula_ = !eula_notifier_->IsEulaAccepted();
  }
}

bool ResourceRequestAllowedNotifier::ResourceRequestsAllowed() {
  if (CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kDisableBackgroundNetworking)) {
    return false;
  }

  // The observer requested permission. Return the current criteria state and
  // set a flag to remind this class to notify the observer once the criteria
  // is met.
  observer_requested_permission_ = true;
  return !waiting_for_user_to_accept_eula_ && !waiting_for_network_;
}

void ResourceRequestAllowedNotifier::SetWaitingForNetworkForTesting(
    bool waiting) {
  waiting_for_network_ = waiting;
}

void ResourceRequestAllowedNotifier::SetWaitingForEulaForTesting(
    bool waiting) {
  waiting_for_user_to_accept_eula_ = waiting;
}

void ResourceRequestAllowedNotifier::MaybeNotifyObserver() {
  // Need to ensure that all criteria are met before notifying observers.
  if (observer_requested_permission_ && ResourceRequestsAllowed()) {
    DVLOG(1) << "Notifying observer of state change.";
    observer_->OnResourceRequestsAllowed();
    // Reset this so the observer is not informed again unless they check
    // ResourceRequestsAllowed again.
    observer_requested_permission_ = false;
  }
}

EulaAcceptedNotifier* ResourceRequestAllowedNotifier::CreateEulaNotifier() {
  return EulaAcceptedNotifier::Create();
}

void ResourceRequestAllowedNotifier::OnEulaAccepted() {
  // This flag should have been set if this was waiting on the EULA
  // notification.
  DCHECK(waiting_for_user_to_accept_eula_);
  DVLOG(1) << "EULA was accepted.";
  waiting_for_user_to_accept_eula_ = false;
  MaybeNotifyObserver();
}

void ResourceRequestAllowedNotifier::OnConnectionTypeChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  // Only attempt to notify observers if this was previously waiting for the
  // network to reconnect, and new network state is actually available. This
  // prevents the notifier from notifying the observer if the notifier was never
  // waiting on the network, or if the network changes from one online state
  // to another (for example, Wifi to 3G, or Wifi to Wifi, if the network were
  // flaky).
  if (waiting_for_network_ &&
      type != net::NetworkChangeNotifier::CONNECTION_NONE) {
    waiting_for_network_ = false;
    DVLOG(1) << "Network came back online.";
    MaybeNotifyObserver();
  }
}
