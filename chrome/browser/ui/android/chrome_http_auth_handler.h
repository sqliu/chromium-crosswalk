// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ANDROID_CHROME_HTTP_AUTH_HANDLER_H_
#define CHROME_BROWSER_UI_ANDROID_CHROME_HTTP_AUTH_HANDLER_H_

#include <jni.h>

#include "base/android/scoped_java_ref.h"
#include "base/string16.h"
#include "chrome/browser/ui/login/login_prompt.h"

// This class facilitates communication between a native LoginHandler
// and a Java land ChromeHttpAuthHandler, which is passed to a
// ContentViewClient to allow it to respond to HTTP authentication requests
// by, e.g., showing the user a login dialog.
class ChromeHttpAuthHandler {
 public:
  ChromeHttpAuthHandler(const string16& explanation);
  ~ChromeHttpAuthHandler();

  // This must be called before using the object.
  // Constructs a corresponding Java land ChromeHttpAuthHandler.
  void Init();

  // Registers an observer to receive callbacks when SetAuth() and CancelAuth()
  // are called. |observer| may be NULL in which case the callbacks are skipped.
  void SetObserver(LoginHandler* observer);

  // Returns a reference to the Java land ChromeHttpAuthHandler.  This
  // reference must not be released, and remains valid until the native
  // ChromeHttpAuthHandler is destructed.
  jobject GetJavaObject();

  // Forwards the autofill data to the Java land object.
  void OnAutofillDataAvailable(
      const string16& username,
      const string16& password);

  // --------------------------------------------------------------
  // JNI Methods
  // --------------------------------------------------------------

  // Submits the username and password to the observer.
  void SetAuth(JNIEnv* env, jobject, jstring username, jstring password);

  // Cancels the authentication attempt of the observer.
  void CancelAuth(JNIEnv* env, jobject);

  // These functions return the strings needed to display a login form.
  base::android::ScopedJavaLocalRef<jstring> GetMessageTitle(
      JNIEnv* env, jobject);
  base::android::ScopedJavaLocalRef<jstring> GetMessageBody(
      JNIEnv* env, jobject);
  base::android::ScopedJavaLocalRef<jstring> GetUsernameLabelText(
      JNIEnv* env, jobject);
  base::android::ScopedJavaLocalRef<jstring> GetPasswordLabelText(
      JNIEnv* env, jobject);
  base::android::ScopedJavaLocalRef<jstring> GetOkButtonText(
      JNIEnv* env, jobject);
  base::android::ScopedJavaLocalRef<jstring> GetCancelButtonText(
      JNIEnv* env, jobject);
  // Registers the ChromeHttpAuthHandler native methods.
  static bool RegisterChromeHttpAuthHandler(JNIEnv* env);
 private:
  LoginHandler* observer_;
  base::android::ScopedJavaGlobalRef<jobject> java_chrome_http_auth_handler_;
  // e.g. "The server example.com:80 requires a username and password."
  string16 explanation_;

  DISALLOW_COPY_AND_ASSIGN(ChromeHttpAuthHandler);
};

#endif  // CHROME_BROWSER_UI_ANDROID_CHROME_HTTP_AUTH_HANDLER_H_
