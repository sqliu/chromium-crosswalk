// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/extensions/install_limiter.h"

#include <string>

#include "base/bind.h"
#include "base/file_util.h"
#include "base/threading/sequenced_worker_pool.h"
#include "chrome/browser/chromeos/extensions/install_limiter_factory.h"
#include "chrome/common/chrome_notification_types.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_source.h"

using content::BrowserThread;

namespace {

// Gets the file size of |file| and stores it in |size| on the blocking pool.
void GetFileSizeOnBlockingPool(const base::FilePath& file, int64* size) {
  DCHECK(BrowserThread::GetBlockingPool()->RunsTasksOnCurrentThread());

  // Get file size. In case of error, sets 0 as file size to let the installer
  // run and fail.
  if (!file_util::GetFileSize(file, size))
    *size = 0;
}

}  // namespace

namespace extensions {

////////////////////////////////////////////////////////////////////////////////
// InstallLimiter::DeferredInstall

InstallLimiter::DeferredInstall::DeferredInstall(
    const scoped_refptr<CrxInstaller>& installer,
    const base::FilePath& path)
    : installer(installer),
      path(path) {
}

InstallLimiter::DeferredInstall::~DeferredInstall() {
}

////////////////////////////////////////////////////////////////////////////////
// InstallLimiter

InstallLimiter* InstallLimiter::Get(Profile* profile) {
  return InstallLimiterFactory::GetForProfile(profile);
}

InstallLimiter::InstallLimiter() : disabled_for_test_(false) {
}

InstallLimiter::~InstallLimiter() {
}

void InstallLimiter::DisableForTest() {
  disabled_for_test_ = true;
}

void InstallLimiter::Add(const scoped_refptr<CrxInstaller>& installer,
                         const base::FilePath& path) {
  // No deferred installs when disabled for test.
  if (disabled_for_test_) {
    installer->InstallCrx(path);
    return;
  }

  int64* size = new int64(0);  // Owned by reply callback below.
  BrowserThread::PostBlockingPoolTaskAndReply(
      FROM_HERE,
      base::Bind(&GetFileSizeOnBlockingPool, path, size),
      base::Bind(&InstallLimiter::AddWithSize, AsWeakPtr(),
                 installer, path, base::Owned(size)));
}

void InstallLimiter::AddWithSize(
    const scoped_refptr<CrxInstaller>& installer,
    const base::FilePath& path,
    int64* size) {
  const int64 kBigAppSizeThreshold = 1048576;  // 1MB

  if (*size <= kBigAppSizeThreshold) {
    RunInstall(installer, path);

    // Stop wait timer and let install notification drive deferred installs.
    wait_timer_.Stop();
    return;
  }

  deferred_installs_.push(DeferredInstall(installer, path));

  // When there are no running installs, wait a bit before running deferred
  // installs to allow small app install to take precedence, especially when a
  // big app is the first one in the list.
  if (running_installers_.empty() && !wait_timer_.IsRunning()) {
    const int kMaxWaitTimeInMs = 5000;  // 5 seconds.
    wait_timer_.Start(
        FROM_HERE,
        base::TimeDelta::FromMilliseconds(kMaxWaitTimeInMs),
        this, &InstallLimiter::CheckAndRunDeferrredInstalls);
  }
}

void InstallLimiter::CheckAndRunDeferrredInstalls() {
  if (deferred_installs_.empty() || !running_installers_.empty())
    return;

  const DeferredInstall& deferred = deferred_installs_.front();
  RunInstall(deferred.installer, deferred.path);
  deferred_installs_.pop();
}

void InstallLimiter::RunInstall(const scoped_refptr<CrxInstaller>& installer,
                                const base::FilePath& path) {
  registrar_.Add(this,
                 chrome::NOTIFICATION_CRX_INSTALLER_DONE,
                 content::Source<CrxInstaller>(installer.get()));

  installer->InstallCrx(path);
  running_installers_.insert(installer);
}

void InstallLimiter::Observe(int type,
                             const content::NotificationSource& source,
                             const content::NotificationDetails& details) {
  DCHECK_EQ(chrome::NOTIFICATION_CRX_INSTALLER_DONE, type);

  registrar_.Remove(this,
                    chrome::NOTIFICATION_CRX_INSTALLER_DONE,
                    source);

  const scoped_refptr<CrxInstaller> installer =
      content::Source<extensions::CrxInstaller>(source).ptr();
  running_installers_.erase(installer);
  CheckAndRunDeferrredInstalls();
}

}  // namespace extensions
