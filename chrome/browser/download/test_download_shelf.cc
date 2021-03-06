// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/download/test_download_shelf.h"

#include "content/public/browser/download_manager.h"

TestDownloadShelf::TestDownloadShelf()
    : is_showing_(false),
      did_add_download_(false) {
}

TestDownloadShelf::~TestDownloadShelf() {
}

bool TestDownloadShelf::IsShowing() const {
  return is_showing_;
}

bool TestDownloadShelf::IsClosing() const {
  return false;
}

Browser* TestDownloadShelf::browser() const {
  return NULL;
}

void TestDownloadShelf::set_download_manager(
    content::DownloadManager* download_manager) {
  download_manager_ = download_manager;
}

void TestDownloadShelf::DoAddDownload(content::DownloadItem* download) {
  did_add_download_ = true;
}

void TestDownloadShelf::DoShow() {
  is_showing_ = true;
}

void TestDownloadShelf::DoClose(CloseReason reason) {
  is_showing_ = false;
}

base::TimeDelta TestDownloadShelf::GetTransientDownloadShowDelay() {
  return base::TimeDelta();
}

content::DownloadManager* TestDownloadShelf::GetDownloadManager() {
  return download_manager_.get();
}
