// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/values.h"
#include "chrome/browser/sync/test/integration/preferences_helper.h"
#include "chrome/browser/sync/test/integration/profile_sync_service_harness.h"
#include "chrome/browser/sync/test/integration/sync_integration_test_util.h"
#include "chrome/browser/sync/test/integration/sync_test.h"
#include "chrome/browser/translate/chrome_translate_client.h"
#include "chrome/common/pref_names.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/sync_driver/pref_names.h"
#include "components/translate/core/browser/translate_prefs.h"
#include "components/translate/core/common/translate_pref_names.h"

using preferences_helper::AppendStringPref;
using preferences_helper::BooleanPrefMatches;
using preferences_helper::ChangeBooleanPref;
using preferences_helper::ChangeIntegerPref;
using preferences_helper::ChangeInt64Pref;
using preferences_helper::ChangeListPref;
using preferences_helper::ChangeStringPref;
using preferences_helper::GetPrefs;
using preferences_helper::IntegerPrefMatches;
using preferences_helper::Int64PrefMatches;
using preferences_helper::ListPrefMatches;
using preferences_helper::StringPrefMatches;
using sync_integration_test_util::AwaitCommitActivityCompletion;

class TwoClientPreferencesSyncTest : public SyncTest {
 public:
  TwoClientPreferencesSyncTest() : SyncTest(TWO_CLIENT) {}
  virtual ~TwoClientPreferencesSyncTest() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(TwoClientPreferencesSyncTest);
};

class LegacyTwoClientPreferencesSyncTest : public SyncTest {
 public:
  LegacyTwoClientPreferencesSyncTest() : SyncTest(TWO_CLIENT_LEGACY) {}
  virtual ~LegacyTwoClientPreferencesSyncTest() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(LegacyTwoClientPreferencesSyncTest);
};

// TCM ID - 7306186.
IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest,
                       kHomePageIsNewTabPage) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kHomePageIsNewTabPage));

  ChangeBooleanPref(0, prefs::kHomePageIsNewTabPage);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kHomePageIsNewTabPage));
}

// TCM ID - 7260488.
IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest, Race) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  DisableVerifier();

  ASSERT_TRUE(StringPrefMatches(prefs::kHomePage));

  ChangeStringPref(0, prefs::kHomePage, "http://www.google.com/0");
  ChangeStringPref(1, prefs::kHomePage,"http://www.google.com/1");
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(StringPrefMatches(prefs::kHomePage));
}

// TCM ID - 3649278.
IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest,
                       kPasswordManagerEnabled) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(
      BooleanPrefMatches(password_manager::prefs::kPasswordManagerEnabled));

  ChangeBooleanPref(0, password_manager::prefs::kPasswordManagerEnabled);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(
      BooleanPrefMatches(password_manager::prefs::kPasswordManagerEnabled));
}

// TCM ID - 3699293.
IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest,
                       kSyncKeepEverythingSynced) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  DisableVerifier();

  ASSERT_TRUE(
      BooleanPrefMatches(sync_driver::prefs::kSyncKeepEverythingSynced));
  ASSERT_TRUE(BooleanPrefMatches(sync_driver::prefs::kSyncThemes));

  GetClient(0)->DisableSyncForDatatype(syncer::THEMES);
  ASSERT_FALSE(
      BooleanPrefMatches(sync_driver::prefs::kSyncKeepEverythingSynced));
}

// TCM ID - 3661290.
IN_PROC_BROWSER_TEST_F(LegacyTwoClientPreferencesSyncTest, DisablePreferences) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  DisableVerifier();

  ASSERT_TRUE(BooleanPrefMatches(sync_driver::prefs::kSyncPreferences));
  ASSERT_TRUE(
      BooleanPrefMatches(password_manager::prefs::kPasswordManagerEnabled));

  GetClient(1)->DisableSyncForDatatype(syncer::PREFERENCES);
  ChangeBooleanPref(0, password_manager::prefs::kPasswordManagerEnabled);
  ASSERT_TRUE(AwaitCommitActivityCompletion(GetSyncService((0))));
  ASSERT_FALSE(
      BooleanPrefMatches(password_manager::prefs::kPasswordManagerEnabled));

  GetClient(1)->EnableSyncForDatatype(syncer::PREFERENCES);
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(
      BooleanPrefMatches(password_manager::prefs::kPasswordManagerEnabled));
}

// TCM ID - 3664292.
IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest, DisableSync) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  DisableVerifier();

  ASSERT_TRUE(BooleanPrefMatches(sync_driver::prefs::kSyncPreferences));
  ASSERT_TRUE(
      BooleanPrefMatches(password_manager::prefs::kPasswordManagerEnabled));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kShowHomeButton));

  GetClient(1)->DisableSyncForAllDatatypes();
  ChangeBooleanPref(0, password_manager::prefs::kPasswordManagerEnabled);
  ASSERT_TRUE(AwaitCommitActivityCompletion(GetSyncService((0))));
  ASSERT_FALSE(
      BooleanPrefMatches(password_manager::prefs::kPasswordManagerEnabled));

  ChangeBooleanPref(1, prefs::kShowHomeButton);
  ASSERT_FALSE(BooleanPrefMatches(prefs::kShowHomeButton));

  GetClient(1)->EnableSyncForAllDatatypes();
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(
      BooleanPrefMatches(password_manager::prefs::kPasswordManagerEnabled));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kShowHomeButton));
}

// TCM ID - 3604297.
IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest, SignInDialog) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  DisableVerifier();

  ASSERT_TRUE(BooleanPrefMatches(sync_driver::prefs::kSyncPreferences));
  ASSERT_TRUE(BooleanPrefMatches(sync_driver::prefs::kSyncBookmarks));
  ASSERT_TRUE(BooleanPrefMatches(sync_driver::prefs::kSyncThemes));
  ASSERT_TRUE(BooleanPrefMatches(sync_driver::prefs::kSyncExtensions));
  ASSERT_TRUE(BooleanPrefMatches(sync_driver::prefs::kSyncAutofill));
  ASSERT_TRUE(
      BooleanPrefMatches(sync_driver::prefs::kSyncKeepEverythingSynced));

  GetClient(0)->DisableSyncForDatatype(syncer::PREFERENCES);
  GetClient(1)->EnableSyncForDatatype(syncer::PREFERENCES);
  GetClient(0)->DisableSyncForDatatype(syncer::AUTOFILL);
  GetClient(1)->EnableSyncForDatatype(syncer::AUTOFILL);
  GetClient(0)->DisableSyncForDatatype(syncer::BOOKMARKS);
  GetClient(1)->EnableSyncForDatatype(syncer::BOOKMARKS);
  GetClient(0)->DisableSyncForDatatype(syncer::EXTENSIONS);
  GetClient(1)->EnableSyncForDatatype(syncer::EXTENSIONS);
  GetClient(0)->DisableSyncForDatatype(syncer::THEMES);
  GetClient(1)->EnableSyncForDatatype(syncer::THEMES);

  ASSERT_TRUE(AwaitQuiescence());

  ASSERT_FALSE(BooleanPrefMatches(sync_driver::prefs::kSyncPreferences));
  ASSERT_FALSE(BooleanPrefMatches(sync_driver::prefs::kSyncBookmarks));
  ASSERT_FALSE(BooleanPrefMatches(sync_driver::prefs::kSyncThemes));
  ASSERT_FALSE(BooleanPrefMatches(sync_driver::prefs::kSyncExtensions));
  ASSERT_FALSE(BooleanPrefMatches(sync_driver::prefs::kSyncAutofill));
  ASSERT_FALSE(
      BooleanPrefMatches(sync_driver::prefs::kSyncKeepEverythingSynced));
}

// TCM ID - 3666296.
IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest, kShowBookmarkBar) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kShowBookmarkBar));

  ChangeBooleanPref(0, prefs::kShowBookmarkBar);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kShowBookmarkBar));
}

// TCM ID - 3611311.
IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest, kCheckDefaultBrowser) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  DisableVerifier();

  ASSERT_TRUE(BooleanPrefMatches(prefs::kCheckDefaultBrowser));

  ChangeBooleanPref(0, prefs::kCheckDefaultBrowser);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_FALSE(BooleanPrefMatches(prefs::kCheckDefaultBrowser));
}

// TCM ID - 3628298.
IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest, kHomePage) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(StringPrefMatches(prefs::kHomePage));

  ChangeStringPref(0, prefs::kHomePage, "http://news.google.com");
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(StringPrefMatches(prefs::kHomePage));
}

// TCM ID - 7297269.
IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest, kShowHomeButton) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kShowHomeButton));

  ChangeBooleanPref(0, prefs::kShowHomeButton);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kShowHomeButton));
}

// TCM ID - 3710285.
IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest, kEnableTranslate) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kEnableTranslate));

  ChangeBooleanPref(0, prefs::kEnableTranslate);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kEnableTranslate));
}

// TCM ID - 3664293.
IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest, kAutofillEnabled) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(autofill::prefs::kAutofillEnabled));

  ChangeBooleanPref(0, autofill::prefs::kAutofillEnabled);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(BooleanPrefMatches(autofill::prefs::kAutofillEnabled));
}

// TCM ID - 3632259.
IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest,
                       kURLsToRestoreOnStartup) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(IntegerPrefMatches(prefs::kRestoreOnStartup));
  ASSERT_TRUE(ListPrefMatches(prefs::kURLsToRestoreOnStartup));

  ChangeIntegerPref(0, prefs::kRestoreOnStartup, 0);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(IntegerPrefMatches(prefs::kRestoreOnStartup));

  base::ListValue urls;
  urls.Append(base::Value::CreateStringValue("http://www.google.com/"));
  urls.Append(base::Value::CreateStringValue("http://www.flickr.com/"));
  ChangeIntegerPref(0, prefs::kRestoreOnStartup, 4);
  ChangeListPref(0, prefs::kURLsToRestoreOnStartup, urls);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(IntegerPrefMatches(prefs::kRestoreOnStartup));
  ASSERT_TRUE(ListPrefMatches(prefs::kURLsToRestoreOnStartup));
}

// TCM ID - 3684287.
IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest, kRestoreOnStartup) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(IntegerPrefMatches(prefs::kRestoreOnStartup));

  ChangeIntegerPref(0, prefs::kRestoreOnStartup, 1);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(IntegerPrefMatches(prefs::kRestoreOnStartup));
}

// TCM ID - 3703314.
IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest, Privacy) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  DisableVerifier();

  ASSERT_TRUE(BooleanPrefMatches(prefs::kAlternateErrorPagesEnabled));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kSearchSuggestEnabled));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kNetworkPredictionEnabled));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kSafeBrowsingEnabled));

  ChangeBooleanPref(0, prefs::kAlternateErrorPagesEnabled);
  ChangeBooleanPref(0, prefs::kSearchSuggestEnabled);
  ChangeBooleanPref(0, prefs::kNetworkPredictionEnabled);
  ChangeBooleanPref(0, prefs::kSafeBrowsingEnabled);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kAlternateErrorPagesEnabled));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kSearchSuggestEnabled));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kNetworkPredictionEnabled));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kSafeBrowsingEnabled));
}

// TCM ID - 3649279.
IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest, ClearData) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  DisableVerifier();

  ASSERT_TRUE(BooleanPrefMatches(prefs::kDeleteBrowsingHistory));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kDeleteDownloadHistory));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kDeleteCache));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kDeleteCookies));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kDeletePasswords));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kDeleteFormData));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kDeleteHostedAppsData));

  ChangeBooleanPref(0, prefs::kDeleteBrowsingHistory);
  ChangeBooleanPref(0, prefs::kDeleteDownloadHistory);
  ChangeBooleanPref(0, prefs::kDeleteCache);
  ChangeBooleanPref(0, prefs::kDeleteCookies);
  ChangeBooleanPref(0, prefs::kDeletePasswords);
  ChangeBooleanPref(0, prefs::kDeleteFormData);
  ChangeBooleanPref(0, prefs::kDeleteHostedAppsData);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kDeleteBrowsingHistory));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kDeleteDownloadHistory));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kDeleteCache));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kDeleteCookies));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kDeletePasswords));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kDeleteFormData));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kDeleteHostedAppsData));
}

// TCM ID - 3686300.
IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest,
                       kWebKitUsesUniversalDetector) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kWebKitUsesUniversalDetector));

  ChangeBooleanPref(0, prefs::kWebKitUsesUniversalDetector);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kWebKitUsesUniversalDetector));
}

// TCM ID - 3673298.
IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest, kDefaultCharset) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(StringPrefMatches(prefs::kDefaultCharset));

  ChangeStringPref(0, prefs::kDefaultCharset, "Thai");
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(StringPrefMatches(prefs::kDefaultCharset));
}

// TCM ID - 3653296.
IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest,
                       kBlockThirdPartyCookies) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kBlockThirdPartyCookies));

  ChangeBooleanPref(0, prefs::kBlockThirdPartyCookies);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kBlockThirdPartyCookies));
}

// TCM ID - 7297279.
IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest,
                       kClearSiteDataOnExit) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kClearSiteDataOnExit));

  ChangeBooleanPref(0, prefs::kClearSiteDataOnExit);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kClearSiteDataOnExit));
}

// TCM ID - 7306184.
IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest,
                       kSafeBrowsingEnabled) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kSafeBrowsingEnabled));

  ChangeBooleanPref(0, prefs::kSafeBrowsingEnabled);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kSafeBrowsingEnabled));
}

// TCM ID - 3624302.
IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest,
                       kAutofillAuxiliaryProfilesEnabled) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  DisableVerifier();

  ASSERT_TRUE(
      BooleanPrefMatches(autofill::prefs::kAutofillAuxiliaryProfilesEnabled));

  ChangeBooleanPref(0, autofill::prefs::kAutofillAuxiliaryProfilesEnabled);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));

  // kAutofillAuxiliaryProfilesEnabled is only synced on Mac.
#if defined(OS_MACOSX)
  ASSERT_TRUE(
      BooleanPrefMatches(autofill::prefs::kAutofillAuxiliaryProfilesEnabled));
#else
  ASSERT_FALSE(
      BooleanPrefMatches(autofill::prefs::kAutofillAuxiliaryProfilesEnabled));
#endif  // OS_MACOSX
}

// TCM ID - 3717298.
IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest, kPromptForDownload) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kPromptForDownload));

  ChangeBooleanPref(0, prefs::kPromptForDownload);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kPromptForDownload));
}

// TCM ID - 3729263.
IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest,
                       kPrefTranslateLanguageBlacklist) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kEnableTranslate));

  scoped_ptr<TranslatePrefs> translate_client0_prefs(
      ChromeTranslateClient::CreateTranslatePrefs(GetPrefs(0)));
  scoped_ptr<TranslatePrefs> translate_client1_prefs(
      ChromeTranslateClient::CreateTranslatePrefs(GetPrefs(1)));
  ASSERT_FALSE(translate_client0_prefs->IsBlockedLanguage("fr"));
  translate_client0_prefs->BlockLanguage("fr");
  ASSERT_TRUE(translate_client0_prefs->IsBlockedLanguage("fr"));

  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(translate_client1_prefs->IsBlockedLanguage("fr"));

  translate_client0_prefs->UnblockLanguage("fr");
  ASSERT_FALSE(translate_client0_prefs->IsBlockedLanguage("fr"));

  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_FALSE(translate_client1_prefs->IsBlockedLanguage("fr"));
}

// TCM ID - 7307195.
IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest,
                       kPrefTranslateWhitelists) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kEnableTranslate));

  scoped_ptr<TranslatePrefs> translate_client0_prefs(
      ChromeTranslateClient::CreateTranslatePrefs(GetPrefs(0)));
  scoped_ptr<TranslatePrefs> translate_client1_prefs(
      ChromeTranslateClient::CreateTranslatePrefs(GetPrefs(1)));
  ASSERT_FALSE(translate_client0_prefs->IsLanguagePairWhitelisted("en", "bg"));
  translate_client0_prefs->WhitelistLanguagePair("en", "bg");
  ASSERT_TRUE(translate_client0_prefs->IsLanguagePairWhitelisted("en", "bg"));

  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(translate_client1_prefs->IsLanguagePairWhitelisted("en", "bg"));

  translate_client0_prefs->RemoveLanguagePairFromWhitelist("en", "bg");
  ASSERT_FALSE(translate_client0_prefs->IsLanguagePairWhitelisted("en", "bg"));

  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_FALSE(translate_client1_prefs->IsLanguagePairWhitelisted("en", "bg"));
}

// TCM ID - 3625298.
IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest,
                       kPrefTranslateSiteBlacklist) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kEnableTranslate));

  GURL url("http://www.google.com");
  std::string host(url.host());
  scoped_ptr<TranslatePrefs> translate_client0_prefs(
      ChromeTranslateClient::CreateTranslatePrefs(GetPrefs(0)));
  scoped_ptr<TranslatePrefs> translate_client1_prefs(
      ChromeTranslateClient::CreateTranslatePrefs(GetPrefs(1)));
  ASSERT_FALSE(translate_client0_prefs->IsSiteBlacklisted(host));
  translate_client0_prefs->BlacklistSite(host);
  ASSERT_TRUE(translate_client0_prefs->IsSiteBlacklisted(host));

  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(translate_client1_prefs->IsSiteBlacklisted(host));

  translate_client0_prefs->RemoveSiteFromBlacklist(host);
  ASSERT_FALSE(translate_client0_prefs->IsSiteBlacklisted(host));

  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_FALSE(translate_client1_prefs->IsSiteBlacklisted(host));
}

// TCM ID - 6515252.
IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest,
                       kExtensionsUIDeveloperMode) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kExtensionsUIDeveloperMode));

  ChangeBooleanPref(0, prefs::kExtensionsUIDeveloperMode);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kExtensionsUIDeveloperMode));
}

// TCM ID - 7583816
IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest, kAcceptLanguages) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  DisableVerifier();
  ASSERT_TRUE(StringPrefMatches(prefs::kAcceptLanguages));

  AppendStringPref(0, prefs::kAcceptLanguages, ",ar");
  AppendStringPref(1, prefs::kAcceptLanguages, ",fr");
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(StringPrefMatches(prefs::kAcceptLanguages));

  ChangeStringPref(0, prefs::kAcceptLanguages, "en-US");
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(StringPrefMatches(prefs::kAcceptLanguages));

  ChangeStringPref(0, prefs::kAcceptLanguages, "ar,en-US");
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(StringPrefMatches(prefs::kAcceptLanguages));
}

// TCM ID - 7590682
#if defined(TOOLKIT_GTK)
IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest, kUsesSystemTheme) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kUsesSystemTheme));

  ChangeBooleanPref(0, prefs::kUsesSystemTheme);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_FALSE(BooleanPrefMatches(prefs::kUsesSystemTheme));
}
#endif  // TOOLKIT_GTK

// TCM ID - 6473347.
#if defined(OS_CHROMEOS)
// Disabled, http://crbug.com/351159 .
IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest, DISABLED_kTapToClickEnabled) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kTapToClickEnabled));

  ChangeBooleanPref(0, prefs::kTapToClickEnabled);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kTapToClickEnabled));

  ChangeBooleanPref(1, prefs::kTapToClickEnabled);
  ASSERT_TRUE(GetClient(1)->AwaitMutualSyncCycleCompletion(GetClient(0)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kTapToClickEnabled));
}
#endif  // OS_CHROMEOS

// TCM ID - 6458824.
#if defined(OS_CHROMEOS)
IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest, kEnableAutoScreenLock) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kEnableAutoScreenLock));

  ChangeBooleanPref(0, prefs::kEnableAutoScreenLock);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kEnableAutoScreenLock));

  ChangeBooleanPref(1, prefs::kEnableAutoScreenLock);
  ASSERT_TRUE(GetClient(1)->AwaitMutualSyncCycleCompletion(GetClient(0)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kEnableAutoScreenLock));
}
#endif  // OS_CHROMEOS

IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest,
                       SingleClientEnabledEncryption) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  ASSERT_TRUE(EnableEncryption(0));
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(IsEncryptionComplete(0));
  ASSERT_TRUE(IsEncryptionComplete(1));
}

IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest,
                       SingleClientEnabledEncryptionAndChanged) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kHomePageIsNewTabPage));

  ChangeBooleanPref(0, prefs::kHomePageIsNewTabPage);
  ASSERT_TRUE(EnableEncryption(0));
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(IsEncryptionComplete(0));
  ASSERT_TRUE(IsEncryptionComplete(1));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kHomePageIsNewTabPage));
}

IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest,
                       BothClientsEnabledEncryption) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  ASSERT_TRUE(EnableEncryption(0));
  ASSERT_TRUE(EnableEncryption(1));
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(IsEncryptionComplete(0));
  ASSERT_TRUE(IsEncryptionComplete(1));
}

IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest,
                       SingleClientEnabledEncryptionBothChanged) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kHomePageIsNewTabPage));
  ASSERT_TRUE(StringPrefMatches(prefs::kHomePage));

  ASSERT_TRUE(EnableEncryption(0));
  ChangeBooleanPref(0, prefs::kHomePageIsNewTabPage);
  ChangeStringPref(1, prefs::kHomePage, "http://www.google.com/1");
  ASSERT_TRUE(AwaitQuiescence());
  ASSERT_TRUE(IsEncryptionComplete(0));
  ASSERT_TRUE(IsEncryptionComplete(1));
  ASSERT_TRUE(BooleanPrefMatches(
      prefs::kHomePageIsNewTabPage));
  ASSERT_TRUE(StringPrefMatches(prefs::kHomePage));
}

IN_PROC_BROWSER_TEST_F(TwoClientPreferencesSyncTest,
                       SingleClientEnabledEncryptionAndChangedMultipleTimes) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  ASSERT_TRUE(BooleanPrefMatches(prefs::kHomePageIsNewTabPage));

  ChangeBooleanPref(0, prefs::kHomePageIsNewTabPage);
  ASSERT_TRUE(EnableEncryption(0));
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(IsEncryptionComplete(0));
  ASSERT_TRUE(IsEncryptionComplete(1));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kHomePageIsNewTabPage));

  ASSERT_TRUE(BooleanPrefMatches(prefs::kShowHomeButton));
  ChangeBooleanPref(0, prefs::kShowHomeButton);
  ASSERT_TRUE(GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1)));
  ASSERT_TRUE(BooleanPrefMatches(prefs::kShowHomeButton));
}
