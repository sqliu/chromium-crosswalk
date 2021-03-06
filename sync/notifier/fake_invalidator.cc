// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sync/notifier/fake_invalidator.h"

namespace syncer {

FakeInvalidator::FakeInvalidator() {}

FakeInvalidator::~FakeInvalidator() {}

bool FakeInvalidator::IsHandlerRegistered(InvalidationHandler* handler) const {
  return registrar_.IsHandlerRegisteredForTest(handler);
}

ObjectIdSet FakeInvalidator::GetRegisteredIds(
    InvalidationHandler* handler) const {
  return registrar_.GetRegisteredIds(handler);
}

const std::string& FakeInvalidator::GetCredentialsEmail() const {
  return email_;
}

const std::string& FakeInvalidator::GetCredentialsToken() const {
  return token_;
}

const ObjectIdInvalidationMap&
FakeInvalidator::GetLastSentInvalidationMap() const {
  return last_sent_invalidation_map_;
}

void FakeInvalidator::EmitOnInvalidatorStateChange(InvalidatorState state) {
  registrar_.UpdateInvalidatorState(state);
}

void FakeInvalidator::EmitOnIncomingInvalidation(
    const ObjectIdInvalidationMap& invalidation_map) {
  registrar_.DispatchInvalidationsToHandlers(invalidation_map);
}

void FakeInvalidator::RegisterHandler(InvalidationHandler* handler) {
  registrar_.RegisterHandler(handler);
}

void FakeInvalidator::UpdateRegisteredIds(InvalidationHandler* handler,
                                          const ObjectIdSet& ids) {
  registrar_.UpdateRegisteredIds(handler, ids);
}

void FakeInvalidator::UnregisterHandler(InvalidationHandler* handler) {
  registrar_.UnregisterHandler(handler);
}

void FakeInvalidator::Acknowledge(const invalidation::ObjectId& id,
                                  const AckHandle& ack_handle) {
  // Do nothing.
}

InvalidatorState FakeInvalidator::GetInvalidatorState() const {
  return registrar_.GetInvalidatorState();
}

void FakeInvalidator::UpdateCredentials(
    const std::string& email, const std::string& token) {
  email_ = email;
  token_ = token;
}

void FakeInvalidator::SendInvalidation(
    const ObjectIdInvalidationMap& invalidation_map) {
  last_sent_invalidation_map_ = invalidation_map;
}

}  // namespace syncer
