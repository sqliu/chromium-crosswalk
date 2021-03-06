// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_TRANSLATE_TRANSLATE_HELPER_H_
#define CHROME_RENDERER_TRANSLATE_TRANSLATE_HELPER_H_

#include <string>

#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "base/time.h"
#include "chrome/common/translate_errors.h"
#include "content/public/renderer/render_view_observer.h"

namespace WebKit {
class WebDocument;
class WebFrame;
}

// This class deals with page translation.
// There is one TranslateHelper per RenderView.

class TranslateHelper : public content::RenderViewObserver {
 public:
  explicit TranslateHelper(content::RenderView* render_view);
  virtual ~TranslateHelper();

  // Informs us that the page's text has been extracted.
  void PageCaptured(const string16& contents);

 protected:
  // The following methods are protected so they can be overridden in
  // unit-tests.
  void OnTranslatePage(int page_id,
                       const std::string& translate_script,
                       const std::string& source_lang,
                       const std::string& target_lang);
  void OnRevertTranslation(int page_id);

  // Returns true if the translate library is available, meaning the JavaScript
  // has already been injected in that page.
  virtual bool IsTranslateLibAvailable();

  // Returns true if the translate library has been initialized successfully.
  virtual bool IsTranslateLibReady();

  // Returns true if the translation script has finished translating the page.
  virtual bool HasTranslationFinished();

  // Returns true if the translation script has reported an error performing the
  // translation.
  virtual bool HasTranslationFailed();

  // Starts the translation by calling the translate library.  This method
  // should only be called when the translate script has been injected in the
  // page.  Returns false if the call failed immediately.
  virtual bool StartTranslation();

  // Asks the Translate element in the page what the language of the page is.
  // Can only be called if a translation has happened and was successful.
  // Returns the language code on success, an empty string on failure.
  virtual std::string GetOriginalPageLanguage();

  // Adjusts a delay time for a posted task. This is used in tests to do tasks
  // immediately by returning 0.
  virtual base::TimeDelta AdjustDelay(int delayInMs);

 private:
  FRIEND_TEST_ALL_PREFIXES(TranslateHelperTest, LanguageCodeTypoCorrection);
  FRIEND_TEST_ALL_PREFIXES(TranslateHelperTest, LanguageCodeSynonyms);
  FRIEND_TEST_ALL_PREFIXES(TranslateHelperTest, ResetInvalidLanguageCode);
  FRIEND_TEST_ALL_PREFIXES(TranslateHelperTest,
                           CLDDisagreeWithWrongLanguageCode);
  FRIEND_TEST_ALL_PREFIXES(TranslateHelperTest,
                           InvalidLanguageMetaTagProviding);

  // Correct language code if it contains well-known mistakes.
  static void CorrectLanguageCodeTypo(std::string* code);

  // Convert language code to the one used in server supporting list.
  static void ConvertLanguageCodeSynonym(std::string* code);

  // Reset language code if the specified string is apparently invalid.
  static void ResetInvalidLanguageCode(std::string* code);

  // Determine content page language from Content-Language code and contents.
  static std::string DeterminePageLanguage(const std::string& code,
                                           const string16& contents);

  // Returns whether the page associated with |document| is a candidate for
  // translation.  Some pages can explictly specify (via a meta-tag) that they
  // should not be translated.
  static bool IsPageTranslatable(WebKit::WebDocument* document);

#if defined(ENABLE_LANGUAGE_DETECTION)
  // Returns the ISO 639_1 language code of the specified |text|, or 'unknown'
  // if it failed.
  static std::string DetermineTextLanguage(const string16& text);
#endif

  // RenderViewObserver implementation.
  virtual bool OnMessageReceived(const IPC::Message& message) OVERRIDE;

  // Cancels any translation that is currently being performed.  This does not
  // revert existing translations.
  void CancelPendingTranslation();

  // Checks if the current running page translation is finished or errored and
  // notifies the browser accordingly.  If the translation has not terminated,
  // posts a task to check again later.
  void CheckTranslateStatus();

  // Executes the JavaScript code in |script| in the main frame of RenderView.
  void ExecuteScript(const std::string& script);

  // Executes the JavaScript code in |script| in the main frame of RenderView,
  // and returns the boolean returned by the script evaluation if the script was
  // run successfully. Otherwise, returns |fallback| value.
  bool ExecuteScriptAndGetBoolResult(const std::string& script, bool fallback);

  // Executes the JavaScript code in |script| in the main frame of RenderView,
  // and returns the string returned by the script evaluation if the script was
  // run successfully. Otherwise, returns empty string.
  std::string ExecuteScriptAndGetStringResult(const std::string& script);

  // Called by TranslatePage to do the actual translation.  |count| is used to
  // limit the number of retries.
  void TranslatePageImpl(int count);

  // Sends a message to the browser to notify it that the translation failed
  // with |error|.
  void NotifyBrowserTranslationFailed(TranslateErrors::Type error);

  // Convenience method to access the main frame.  Can return NULL, typically
  // if the page is being closed.
  WebKit::WebFrame* GetMainFrame();

  // The states associated with the current translation.
  bool translation_pending_;
  int page_id_;
  std::string source_lang_;
  std::string target_lang_;

  // Method factory used to make calls to TranslatePageImpl.
  base::WeakPtrFactory<TranslateHelper> weak_method_factory_;

  DISALLOW_COPY_AND_ASSIGN(TranslateHelper);
};

#endif  // CHROME_RENDERER_TRANSLATE_TRANSLATE_HELPER_H_
