// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/extensions/extension_install_ui.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/startup_helper.h"
#include "chrome/browser/extensions/webstore_installer_test.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/managed_mode/managed_user_service.h"
#include "chrome/browser/managed_mode/managed_user_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/test_switches.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "extensions/browser/extension_host.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/value_builder.h"
#include "net/dns/mock_host_resolver.h"
#include "url/gurl.h"

using content::WebContents;
using extensions::DictionaryBuilder;
using extensions::Extension;
using extensions::ExtensionBuilder;
using extensions::ListBuilder;

const char kWebstoreDomain[] = "cws.com";
const char kAppDomain[] = "app.com";
const char kNonAppDomain[] = "nonapp.com";
const char kTestExtensionId[] = "ecglahbcnmdpdciemllbhojghbkagdje";
const char kTestDataPath[] = "extensions/api_test/webstore_inline_install";
const char kCrxFilename[] = "extension.crx";

class WebstoreStartupInstallerTest : public WebstoreInstallerTest {
 public:
  WebstoreStartupInstallerTest()
      : WebstoreInstallerTest(
            kWebstoreDomain,
            kTestDataPath,
            kCrxFilename,
            kAppDomain,
            kNonAppDomain) {}
};

IN_PROC_BROWSER_TEST_F(WebstoreStartupInstallerTest, Install) {
  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      switches::kAppsGalleryInstallAutoConfirmForTests, "accept");

  ui_test_utils::NavigateToURL(
      browser(), GenerateTestServerUrl(kAppDomain, "install.html"));

  RunTest("runTest");

  const extensions::Extension* extension = browser()->profile()->
      GetExtensionService()->GetExtensionById(kTestExtensionId, false);
  EXPECT_TRUE(extension);
}

IN_PROC_BROWSER_TEST_F(WebstoreStartupInstallerTest,
    InstallNotAllowedFromNonVerifiedDomains) {
  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      switches::kAppsGalleryInstallAutoConfirmForTests, "cancel");
  ui_test_utils::NavigateToURL(
      browser(),
      GenerateTestServerUrl(kNonAppDomain, "install_non_verified_domain.html"));

  RunTest("runTest1");
  RunTest("runTest2");
}

IN_PROC_BROWSER_TEST_F(WebstoreStartupInstallerTest, FindLink) {
  ui_test_utils::NavigateToURL(
      browser(), GenerateTestServerUrl(kAppDomain, "find_link.html"));

  RunTest("runTest");
}

// Flakes on all platforms: http://crbug.com/95713, http://crbug.com/229947
IN_PROC_BROWSER_TEST_F(WebstoreStartupInstallerTest,
                       DISABLED_ArgumentValidation) {
  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      switches::kAppsGalleryInstallAutoConfirmForTests, "cancel");

  // Each of these tests has to run separately, since one page/tab can
  // only have one in-progress install request. These tests don't all pass
  // callbacks to install, so they have no way to wait for the installation
  // to complete before starting the next test.
  bool is_finished = false;
  for (int i = 0; !is_finished; ++i) {
    ui_test_utils::NavigateToURL(
        browser(),
        GenerateTestServerUrl(kAppDomain, "argument_validation.html"));
    is_finished = !RunIndexedTest("runTest", i);
  }
}

IN_PROC_BROWSER_TEST_F(WebstoreStartupInstallerTest, MultipleInstallCalls) {
  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      switches::kAppsGalleryInstallAutoConfirmForTests, "cancel");

  ui_test_utils::NavigateToURL(
      browser(),
      GenerateTestServerUrl(kAppDomain, "multiple_install_calls.html"));
  RunTest("runTest");
}

IN_PROC_BROWSER_TEST_F(WebstoreStartupInstallerTest, InstallNotSupported) {
  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      switches::kAppsGalleryInstallAutoConfirmForTests, "cancel");
  ui_test_utils::NavigateToURL(
      browser(),
      GenerateTestServerUrl(kAppDomain, "install_not_supported.html"));

  ui_test_utils::WindowedTabAddedNotificationObserver observer(
      content::NotificationService::AllSources());
  RunTest("runTest");
  observer.Wait();

  // The inline install should fail, and a store-provided URL should be opened
  // in a new tab.
  WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_EQ(GURL("http://cws.com/show-me-the-money"), web_contents->GetURL());
}

// Regression test for http://crbug.com/144991.
IN_PROC_BROWSER_TEST_F(WebstoreStartupInstallerTest, InstallFromHostedApp) {
  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      switches::kAppsGalleryInstallAutoConfirmForTests, "accept");

  const GURL kInstallUrl = GenerateTestServerUrl(kAppDomain, "install.html");

  // We're forced to construct a hosted app dynamically because we need the
  // app to run on a declared URL, but we don't know the port ahead of time.
  scoped_refptr<const Extension> hosted_app = ExtensionBuilder()
      .SetManifest(DictionaryBuilder()
          .Set("name", "hosted app")
          .Set("version", "1")
          .Set("app", DictionaryBuilder()
              .Set("urls", ListBuilder().Append(kInstallUrl.spec()))
              .Set("launch", DictionaryBuilder()
                  .Set("web_url", kInstallUrl.spec())))
          .Set("manifest_version", 2))
      .Build();
  ASSERT_TRUE(hosted_app.get());

  ExtensionService* extension_service =
      extensions::ExtensionSystem::Get(browser()->profile())->
          extension_service();

  extension_service->AddExtension(hosted_app.get());
  EXPECT_TRUE(extension_service->extensions()->Contains(hosted_app->id()));

  ui_test_utils::NavigateToURL(browser(), kInstallUrl);

  EXPECT_FALSE(extension_service->extensions()->Contains(kTestExtensionId));
  RunTest("runTest");
  EXPECT_TRUE(extension_service->extensions()->Contains(kTestExtensionId));
}

class WebstoreStartupInstallerManagedUsersTest
    : public WebstoreStartupInstallerTest {
 public:
  // InProcessBrowserTest overrides:
  virtual void SetUpCommandLine(base::CommandLine* command_line) OVERRIDE {
    WebstoreStartupInstallerTest::SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII(switches::kManagedUserId, "asdf");
  }
};

IN_PROC_BROWSER_TEST_F(WebstoreStartupInstallerManagedUsersTest,
                       InstallProhibited) {
#if defined(OS_WIN) && defined(USE_ASH)
  // Disable this test in Metro+Ash for now (http://crbug.com/262796).
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kAshBrowserTests))
    return;
#endif

  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      switches::kAppsGalleryInstallAutoConfirmForTests, "accept");

  ui_test_utils::NavigateToURL(
      browser(), GenerateTestServerUrl(kAppDomain, "install_prohibited.html"));

  RunTest("runTest");

  // No error infobar should show up.
  WebContents* contents = browser()->tab_strip_model()->GetActiveWebContents();
  InfoBarService* infobar_service = InfoBarService::FromWebContents(contents);
  EXPECT_EQ(0u, infobar_service->infobar_count());
}

// The unpack failure test needs to use a different install .crx, which is
// specified via a command-line flag, so it needs its own test subclass.
class WebstoreStartupInstallUnpackFailureTest
    : public WebstoreStartupInstallerTest {
 public:
  virtual void SetUpCommandLine(base::CommandLine* command_line) OVERRIDE {
    WebstoreStartupInstallerTest::SetUpCommandLine(command_line);

    GURL crx_url = GenerateTestServerUrl(
        kWebstoreDomain, "malformed_extension.crx");
    base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
        switches::kAppsGalleryUpdateURL, crx_url.spec());
  }

  virtual void SetUpInProcessBrowserTestFixture() OVERRIDE {
    WebstoreStartupInstallerTest::SetUpInProcessBrowserTestFixture();
    ExtensionInstallUI::set_disable_failure_ui_for_tests();
  }
};

IN_PROC_BROWSER_TEST_F(WebstoreStartupInstallUnpackFailureTest,
    WebstoreStartupInstallUnpackFailureTest) {
  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      switches::kAppsGalleryInstallAutoConfirmForTests, "accept");

  ui_test_utils::NavigateToURL(browser(),
      GenerateTestServerUrl(kAppDomain, "install_unpack_failure.html"));

  RunTest("runTest");
}

class CommandLineWebstoreInstall : public WebstoreStartupInstallerTest,
                                   public content::NotificationObserver {
 public:
  CommandLineWebstoreInstall() : saw_install_(false), browser_open_count_(0) {}
  virtual ~CommandLineWebstoreInstall() {}

  virtual void SetUpOnMainThread() OVERRIDE {
    WebstoreStartupInstallerTest::SetUpOnMainThread();
    registrar_.Add(this,
                   chrome::NOTIFICATION_EXTENSION_INSTALLED_DEPRECATED,
                   content::NotificationService::AllSources());
    registrar_.Add(this, chrome::NOTIFICATION_BROWSER_OPENED,
                   content::NotificationService::AllSources());
  }

  bool saw_install() { return saw_install_; }

  int browser_open_count() { return browser_open_count_; }

  // NotificationObserver interface.
  virtual void Observe(int type,
                       const content::NotificationSource& source,
                       const content::NotificationDetails& details) OVERRIDE {
    if (type == chrome::NOTIFICATION_EXTENSION_INSTALLED_DEPRECATED) {
      const Extension* extension =
          content::Details<const extensions::InstalledExtensionInfo>(details)->
              extension;
      ASSERT_TRUE(extension != NULL);
      EXPECT_EQ(extension->id(), kTestExtensionId);
      saw_install_ = true;
    } else if (type == chrome::NOTIFICATION_BROWSER_OPENED) {
      browser_open_count_++;
    } else {
      ASSERT_TRUE(false) << "Unexpected notification type : " << type;
    }
  }

  content::NotificationRegistrar registrar_;

  // Have we seen an installation notification for kTestExtensionId ?
  bool saw_install_;

  // How many NOTIFICATION_BROWSER_OPENED notifications have we seen?
  int browser_open_count_;
};

IN_PROC_BROWSER_TEST_F(CommandLineWebstoreInstall, Accept) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  command_line->AppendSwitchASCII(
      switches::kInstallFromWebstore, kTestExtensionId);
  command_line->AppendSwitchASCII(
      switches::kAppsGalleryInstallAutoConfirmForTests, "accept");
  extensions::StartupHelper helper;
  EXPECT_TRUE(helper.InstallFromWebstore(*command_line, browser()->profile()));
  EXPECT_TRUE(saw_install());
  EXPECT_EQ(0, browser_open_count());
}

IN_PROC_BROWSER_TEST_F(CommandLineWebstoreInstall, Cancel) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  command_line->AppendSwitchASCII(
      switches::kInstallFromWebstore, kTestExtensionId);
  command_line->AppendSwitchASCII(
      switches::kAppsGalleryInstallAutoConfirmForTests, "cancel");
  extensions::StartupHelper helper;
  EXPECT_FALSE(helper.InstallFromWebstore(*command_line, browser()->profile()));
  EXPECT_FALSE(saw_install());
  EXPECT_EQ(0, browser_open_count());
}

IN_PROC_BROWSER_TEST_F(CommandLineWebstoreInstall, LimitedAccept) {
  extensions::StartupHelper helper;

  // Small test of "WebStoreIdFromLimitedInstallCmdLine" which made more
  // sense together with the rest of the test for "LimitedInstallFromWebstore".
  base::CommandLine command_line_test1(base::CommandLine::NO_PROGRAM);
  command_line_test1.AppendSwitchASCII(switches::kLimitedInstallFromWebstore,
      "1");
  EXPECT_EQ("nckgahadagoaajjgafhacjanaoiihapd",
      helper.WebStoreIdFromLimitedInstallCmdLine(command_line_test1));

  base::CommandLine command_line_test2(base::CommandLine::NO_PROGRAM);
  command_line_test1.AppendSwitchASCII(switches::kLimitedInstallFromWebstore,
      "2");
  EXPECT_EQ(kTestExtensionId,
      helper.WebStoreIdFromLimitedInstallCmdLine(command_line_test1));

  // Now, on to the real test for LimitedInstallFromWebstore.
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  command_line->AppendSwitchASCII(
      switches::kLimitedInstallFromWebstore, "2");
  helper.LimitedInstallFromWebstore(*command_line, browser()->profile(),
      base::MessageLoop::QuitWhenIdleClosure());
  base::MessageLoop::current()->Run();

  EXPECT_TRUE(saw_install());
  EXPECT_EQ(0, browser_open_count());
}
