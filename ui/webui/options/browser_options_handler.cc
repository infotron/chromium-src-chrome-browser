// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/options/browser_options_handler.h"

#include <string>
#include <vector>

#include "apps/shell_window.h"
#include "apps/shell_window_registry.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/memory/singleton.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/histogram.h"
#include "base/path_service.h"
#include "base/prefs/pref_service.h"
#include "base/prefs/scoped_user_pref_update.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/value_conversions.h"
#include "base/values.h"
#include "chrome/browser/auto_launch_trial.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chrome_page_zoom.h"
#include "chrome/browser/custom_home_pages_table_model.h"
#include "chrome/browser/download/download_prefs.h"
#include "chrome/browser/gpu/gpu_mode_manager.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/prefs/session_startup_pref.h"
#include "chrome/browser/printing/cloud_print/cloud_print_proxy_service.h"
#include "chrome/browser/printing/cloud_print/cloud_print_proxy_service_factory.h"
#include "chrome/browser/printing/cloud_print/cloud_print_url.h"
#include "chrome/browser/profile_resetter/automatic_profile_resetter.h"
#include "chrome/browser/profile_resetter/automatic_profile_resetter_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_info_cache.h"
#include "chrome/browser/profiles/profile_info_util.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_metrics.h"
#include "chrome/browser/profiles/profile_shortcut_manager.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/profiles/profiles_state.h"
#include "chrome/browser/search/hotword_service.h"
#include "chrome/browser/search/hotword_service_factory.h"
#include "chrome/browser/search/search.h"
#include "chrome/browser/search_engines/template_url.h"
#include "chrome/browser/search_engines/template_url_service.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/signin/signin_manager.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/sync/profile_sync_service.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/browser/sync/sync_ui_util.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/chrome_select_file_policy.h"
#include "chrome/browser/ui/host_desktop.h"
#include "chrome/browser/ui/options/options_util.h"
#include "chrome/browser/ui/webui/favicon_source.h"
#include "chrome/browser/ui/webui/options/options_handlers_helper.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/net/url_fixer_upper.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "chromeos/chromeos_switches.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/download_manager.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/user_metrics.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_view.h"
#include "content/public/common/page_zoom.h"
#include "google_apis/gaia/gaia_auth_util.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "grit/chromium_strings.h"
#include "grit/generated_resources.h"
#include "grit/locale_settings.h"
#include "grit/theme_resources.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/webui/web_ui_util.h"

#if !defined(OS_CHROMEOS)
#include "chrome/browser/ui/webui/options/advanced_options_utils.h"
#endif

#if defined(OS_CHROMEOS)
#include "ash/ash_switches.h"
#include "ash/magnifier/magnifier_constants.h"
#include "chrome/browser/chromeos/accessibility/accessibility_util.h"
#include "chrome/browser/chromeos/extensions/wallpaper_manager_util.h"
#include "chrome/browser/chromeos/login/user.h"
#include "chrome/browser/chromeos/login/user_manager.h"
#include "chrome/browser/chromeos/policy/browser_policy_connector_chromeos.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/chromeos/system/timezone_util.h"
#include "chrome/browser/policy/profile_policy_connector.h"
#include "chrome/browser/policy/profile_policy_connector_factory.h"
#include "chrome/browser/ui/browser_window.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/power_manager_client.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_namespace.h"
#include "components/policy/core/common/policy_service.h"
#include "policy/policy_constants.h"
#include "ui/gfx/image/image_skia.h"
#endif  // defined(OS_CHROMEOS)

#if defined(OS_WIN)
#include "chrome/installer/util/auto_launch_util.h"
#endif  // defined(OS_WIN)

#if defined(ENABLE_MDNS)
#include "chrome/browser/local_discovery/privet_notifications.h"
#endif

using base::UserMetricsAction;
using content::BrowserContext;
using content::BrowserThread;
using content::DownloadManager;
using content::OpenURLParams;
using content::Referrer;

namespace options {

namespace {

bool ShouldShowMultiProfilesUserList(chrome::HostDesktopType desktop_type) {
#if defined(OS_CHROMEOS)
  // On Chrome OS we use different UI for multi-profiles.
  return false;
#else
  if (desktop_type != chrome::HOST_DESKTOP_TYPE_NATIVE)
    return false;
  return profiles::IsMultipleProfilesEnabled();
#endif
}

}  // namespace

BrowserOptionsHandler::BrowserOptionsHandler()
    : page_initialized_(false),
      template_url_service_(NULL),
      cloud_print_mdns_ui_enabled_(false),
      weak_ptr_factory_(this) {
#if !defined(OS_MACOSX)
  default_browser_worker_ = new ShellIntegration::DefaultBrowserWorker(this);
#endif

#if defined(ENABLE_FULL_PRINTING)
#if !defined(GOOGLE_CHROME_BUILD) && defined(OS_WIN)
  // On Windows, we need the PDF plugin which is only guaranteed to exist on
  // Google Chrome builds. Use a command-line switch for Windows non-Google
  //  Chrome builds.
  cloud_print_connector_ui_enabled_ =
      CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kEnableCloudPrintProxy);
#elif !defined(OS_CHROMEOS)
  // Always enabled for Mac, Linux and Google Chrome Windows builds.
  // Never enabled for Chrome OS, we don't even need to indicate it.
  cloud_print_connector_ui_enabled_ = true;
#endif
#endif  // defined(ENABLE_FULL_PRINTING)

#if defined(ENABLE_MDNS)
  cloud_print_mdns_ui_enabled_ = !CommandLine::ForCurrentProcess()->HasSwitch(
        switches::kDisableDeviceDiscovery);
#endif  // defined(ENABLE_MDNS)
}

BrowserOptionsHandler::~BrowserOptionsHandler() {
  ProfileSyncService* sync_service(ProfileSyncServiceFactory::
      GetInstance()->GetForProfile(Profile::FromWebUI(web_ui())));
  if (sync_service)
    sync_service->RemoveObserver(this);

  if (default_browser_worker_.get())
    default_browser_worker_->ObserverDestroyed();
  if (template_url_service_)
    template_url_service_->RemoveObserver(this);
  // There may be pending file dialogs, we need to tell them that we've gone
  // away so they don't try and call back to us.
  if (select_folder_dialog_.get())
    select_folder_dialog_->ListenerDestroyed();
}

void BrowserOptionsHandler::GetLocalizedValues(base::DictionaryValue* values) {
  DCHECK(values);

  static OptionsStringResource resources[] = {
    { "advancedSectionTitleCloudPrint", IDS_GOOGLE_CLOUD_PRINT },
    { "currentUserOnly", IDS_OPTIONS_CURRENT_USER_ONLY },
    { "advancedSectionTitleContent",
      IDS_OPTIONS_ADVANCED_SECTION_TITLE_CONTENT },
    { "advancedSectionTitleLanguages",
      IDS_OPTIONS_ADVANCED_SECTION_TITLE_LANGUAGES },
    { "advancedSectionTitleNetwork",
      IDS_OPTIONS_ADVANCED_SECTION_TITLE_NETWORK },
    { "advancedSectionTitlePrivacy",
      IDS_OPTIONS_ADVANCED_SECTION_TITLE_PRIVACY },
    { "advancedSectionTitleSecurity",
      IDS_OPTIONS_ADVANCED_SECTION_TITLE_SECURITY },
    { "autofillEnabled", IDS_OPTIONS_AUTOFILL_ENABLE },
    { "autologinEnabled", IDS_OPTIONS_PASSWORDS_AUTOLOGIN },
    { "autoOpenFileTypesInfo", IDS_OPTIONS_OPEN_FILE_TYPES_AUTOMATICALLY },
    { "autoOpenFileTypesResetToDefault",
      IDS_OPTIONS_AUTOOPENFILETYPES_RESETTODEFAULT },
    { "changeHomePage", IDS_OPTIONS_CHANGE_HOME_PAGE },
    { "certificatesManageButton", IDS_OPTIONS_CERTIFICATES_MANAGE_BUTTON },
    { "customizeSync", IDS_OPTIONS_CUSTOMIZE_SYNC_BUTTON_LABEL },
    { "defaultFontSizeLabel", IDS_OPTIONS_DEFAULT_FONT_SIZE_LABEL },
    { "defaultSearchManageEngines", IDS_OPTIONS_DEFAULTSEARCH_MANAGE_ENGINES },
    { "defaultZoomFactorLabel", IDS_OPTIONS_DEFAULT_ZOOM_LEVEL_LABEL },
#if defined(OS_CHROMEOS)
    { "disableGData", IDS_OPTIONS_DISABLE_GDATA },
#endif
    { "disableWebServices", IDS_OPTIONS_DISABLE_WEB_SERVICES },
#if defined(OS_CHROMEOS)
    { "displayOptions",
      IDS_OPTIONS_SETTINGS_DISPLAY_OPTIONS_BUTTON_LABEL },
#endif
    { "doNotTrack", IDS_OPTIONS_ENABLE_DO_NOT_TRACK },
    { "doNotTrackConfirmMessage", IDS_OPTIONS_ENABLE_DO_NOT_TRACK_BUBBLE_TEXT },
    { "doNotTrackConfirmEnable",
       IDS_OPTIONS_ENABLE_DO_NOT_TRACK_BUBBLE_ENABLE },
    { "doNotTrackConfirmDisable",
       IDS_OPTIONS_ENABLE_DO_NOT_TRACK_BUBBLE_DISABLE },
    { "downloadLocationAskForSaveLocation",
      IDS_OPTIONS_DOWNLOADLOCATION_ASKFORSAVELOCATION },
    { "downloadLocationBrowseTitle",
      IDS_OPTIONS_DOWNLOADLOCATION_BROWSE_TITLE },
    { "downloadLocationChangeButton",
      IDS_OPTIONS_DOWNLOADLOCATION_CHANGE_BUTTON },
    { "downloadLocationGroupName", IDS_OPTIONS_DOWNLOADLOCATION_GROUP_NAME },
    { "enableLogging", IDS_OPTIONS_ENABLE_LOGGING },
    { "fontSettingsCustomizeFontsButton",
      IDS_OPTIONS_FONTSETTINGS_CUSTOMIZE_FONTS_BUTTON },
    { "fontSizeLabelCustom", IDS_OPTIONS_FONT_SIZE_LABEL_CUSTOM },
    { "fontSizeLabelLarge", IDS_OPTIONS_FONT_SIZE_LABEL_LARGE },
    { "fontSizeLabelMedium", IDS_OPTIONS_FONT_SIZE_LABEL_MEDIUM },
    { "fontSizeLabelSmall", IDS_OPTIONS_FONT_SIZE_LABEL_SMALL },
    { "fontSizeLabelVeryLarge", IDS_OPTIONS_FONT_SIZE_LABEL_VERY_LARGE },
    { "fontSizeLabelVerySmall", IDS_OPTIONS_FONT_SIZE_LABEL_VERY_SMALL },
    { "hideAdvancedSettings", IDS_SETTINGS_HIDE_ADVANCED_SETTINGS },
    { "homePageNtp", IDS_OPTIONS_HOMEPAGE_NTP },
    { "homePageShowHomeButton", IDS_OPTIONS_TOOLBAR_SHOW_HOME_BUTTON },
    { "homePageUseNewTab", IDS_OPTIONS_HOMEPAGE_USE_NEWTAB },
    { "homePageUseURL", IDS_OPTIONS_HOMEPAGE_USE_URL },
    { "hotwordSearchEnable", IDS_HOTWORD_SEARCH_PREF_CHKBOX },
    { "hotwordConfirmEnable", IDS_HOTWORD_CONFIRM_BUBBLE_ENABLE },
    { "hotwordConfirmDisable", IDS_HOTWORD_CONFIRM_BUBBLE_DISABLE },
    { "hotwordConfirmMessage", IDS_HOTWORD_SEARCH_PREF_DESCRIPTION },
    { "importData", IDS_OPTIONS_IMPORT_DATA_BUTTON },
    { "improveBrowsingExperience", IDS_OPTIONS_IMPROVE_BROWSING_EXPERIENCE },
    { "languageAndSpellCheckSettingsButton",
      IDS_OPTIONS_SETTINGS_LANGUAGE_AND_INPUT_SETTINGS },
    { "linkDoctorPref", IDS_OPTIONS_LINKDOCTOR_PREF },
    { "manageAutofillSettings", IDS_OPTIONS_MANAGE_AUTOFILL_SETTINGS_LINK },
    { "manageLanguages", IDS_OPTIONS_TRANSLATE_MANAGE_LANGUAGES },
    { "managePasswords", IDS_OPTIONS_PASSWORDS_MANAGE_PASSWORDS_LINK },
    { "networkPredictionEnabledDescription",
      IDS_NETWORK_PREDICTION_ENABLED_DESCRIPTION },
    { "passwordsAndAutofillGroupName",
      IDS_OPTIONS_PASSWORDS_AND_FORMS_GROUP_NAME },
    { "passwordManagerEnabled", IDS_OPTIONS_PASSWORD_MANAGER_ENABLE },
    { "privacyClearDataButton", IDS_OPTIONS_PRIVACY_CLEAR_DATA_BUTTON },
    { "privacyContentSettingsButton",
      IDS_OPTIONS_PRIVACY_CONTENT_SETTINGS_BUTTON },
    { "profilesCreate", IDS_PROFILES_CREATE_BUTTON_LABEL },
    { "profilesDelete", IDS_PROFILES_DELETE_BUTTON_LABEL },
    { "profilesDeleteSingle", IDS_PROFILES_DELETE_SINGLE_BUTTON_LABEL },
    { "profilesListItemCurrent", IDS_PROFILES_LIST_ITEM_CURRENT },
    { "profilesManage", IDS_PROFILES_MANAGE_BUTTON_LABEL },
    { "profilesSupervisedDashboardTip",
      IDS_PROFILES_SUPERVISED_USER_DASHBOARD_TIP },
#if defined(ENABLE_SETTINGS_APP)
    { "profilesAppListSwitch", IDS_SETTINGS_APP_PROFILES_SWITCH_BUTTON_LABEL },
#endif
    { "proxiesLabelExtension", IDS_OPTIONS_EXTENSION_PROXIES_LABEL },
    { "proxiesLabelSystem", IDS_OPTIONS_SYSTEM_PROXIES_LABEL,
      IDS_PRODUCT_NAME },
    { "resetProfileSettings", IDS_RESET_PROFILE_SETTINGS_BUTTON },
    { "resetProfileSettingsDescription",
      IDS_RESET_PROFILE_SETTINGS_DESCRIPTION },
    { "resetProfileSettingsSectionTitle",
      IDS_RESET_PROFILE_SETTINGS_SECTION_TITLE },
    { "safeBrowsingEnableProtection",
      IDS_OPTIONS_SAFEBROWSING_ENABLEPROTECTION },
    { "sectionTitleAppearance", IDS_APPEARANCE_GROUP_NAME },
    { "sectionTitleDefaultBrowser", IDS_OPTIONS_DEFAULTBROWSER_GROUP_NAME },
    { "sectionTitleUsers", IDS_PROFILES_OPTIONS_GROUP_NAME },
    { "sectionTitleSearch", IDS_OPTIONS_DEFAULTSEARCH_GROUP_NAME },
    { "sectionTitleStartup", IDS_OPTIONS_STARTUP_GROUP_NAME },
    { "sectionTitleSync", IDS_SYNC_OPTIONS_GROUP_NAME },
    { "spellingConfirmMessage", IDS_CONTENT_CONTEXT_SPELLING_BUBBLE_TEXT },
    { "spellingConfirmEnable", IDS_CONTENT_CONTEXT_SPELLING_BUBBLE_ENABLE },
    { "spellingConfirmDisable", IDS_CONTENT_CONTEXT_SPELLING_BUBBLE_DISABLE },
    { "spellingPref", IDS_OPTIONS_SPELLING_PREF },
    { "startupRestoreLastSession", IDS_OPTIONS_STARTUP_RESTORE_LAST_SESSION },
    { "settingsTitle", IDS_SETTINGS_TITLE },
    { "showAdvancedSettings", IDS_SETTINGS_SHOW_ADVANCED_SETTINGS },
    { "sslCheckRevocation", IDS_OPTIONS_SSL_CHECKREVOCATION },
    { "startupSetPages", IDS_OPTIONS_STARTUP_SET_PAGES },
    { "startupShowNewTab", IDS_OPTIONS_STARTUP_SHOW_NEWTAB },
    { "startupShowPages", IDS_OPTIONS_STARTUP_SHOW_PAGES },
    { "suggestPref", IDS_OPTIONS_SUGGEST_PREF },
    { "syncButtonTextInProgress", IDS_SYNC_NTP_SETUP_IN_PROGRESS },
    { "syncButtonTextStop", IDS_SYNC_STOP_SYNCING_BUTTON_LABEL },
    { "themesGallery", IDS_THEMES_GALLERY_BUTTON },
    { "themesGalleryURL", IDS_THEMES_GALLERY_URL },
    { "tabsToLinksPref", IDS_OPTIONS_TABS_TO_LINKS_PREF },
    { "toolbarShowBookmarksBar", IDS_OPTIONS_TOOLBAR_SHOW_BOOKMARKS_BAR },
    { "toolbarShowHomeButton", IDS_OPTIONS_TOOLBAR_SHOW_HOME_BUTTON },
    { "translateEnableTranslate",
      IDS_OPTIONS_TRANSLATE_ENABLE_TRANSLATE },
#if defined(OS_LINUX) && !defined(OS_CHROMEOS)
    { "showWindowDecorations", IDS_SHOW_WINDOW_DECORATIONS },
    { "themesNativeButton", IDS_THEMES_GTK_BUTTON },
    { "themesSetClassic", IDS_THEMES_SET_CLASSIC },
#else
    { "themes", IDS_THEMES_GROUP_NAME },
#endif
    { "themesReset", IDS_THEMES_RESET_BUTTON },
#if defined(OS_CHROMEOS)
    { "accessibilityExplanation",
      IDS_OPTIONS_SETTINGS_ACCESSIBILITY_EXPLANATION },
    { "accessibilitySettings",
      IDS_OPTIONS_SETTINGS_ACCESSIBILITY_SETTINGS },
    { "accessibilityHighContrast",
      IDS_OPTIONS_SETTINGS_ACCESSIBILITY_HIGH_CONTRAST_DESCRIPTION },
    { "accessibilityScreenMagnifier",
      IDS_OPTIONS_SETTINGS_ACCESSIBILITY_SCREEN_MAGNIFIER_DESCRIPTION },
    { "accessibilityTapDragging",
      IDS_OPTIONS_SETTINGS_ACCESSIBILITY_TOUCHPAD_TAP_DRAGGING_DESCRIPTION },
    { "accessibilityScreenMagnifierOff",
      IDS_OPTIONS_SETTINGS_ACCESSIBILITY_SCREEN_MAGNIFIER_OFF },
    { "accessibilityScreenMagnifierFull",
      IDS_OPTIONS_SETTINGS_ACCESSIBILITY_SCREEN_MAGNIFIER_FULL },
    { "accessibilityScreenMagnifierPartial",
      IDS_OPTIONS_SETTINGS_ACCESSIBILITY_SCREEN_MAGNIFIER_PARTIAL },
    { "accessibilityLargeCursor",
      IDS_OPTIONS_SETTINGS_ACCESSIBILITY_LARGE_CURSOR_DESCRIPTION },
    { "accessibilityStickyKeys",
      IDS_OPTIONS_SETTINGS_ACCESSIBILITY_STICKY_KEYS_DESCRIPTION },
    { "accessibilitySpokenFeedback",
      IDS_OPTIONS_SETTINGS_ACCESSIBILITY_SPOKEN_FEEDBACK_DESCRIPTION },
    { "accessibilityTitle",
      IDS_OPTIONS_SETTINGS_SECTION_TITLE_ACCESSIBILITY },
    { "accessibilityVirtualKeyboard",
      IDS_OPTIONS_SETTINGS_ACCESSIBILITY_VIRTUAL_KEYBOARD_DESCRIPTION },
    { "accessibilityAlwaysShowMenu",
      IDS_OPTIONS_SETTINGS_ACCESSIBILITY_SHOULD_ALWAYS_SHOW_MENU },
    { "accessibilityAutoclick",
      IDS_OPTIONS_SETTINGS_ACCESSIBILITY_AUTOCLICK_DESCRIPTION },
    { "accessibilityAutoclickDropdown",
      IDS_OPTIONS_SETTINGS_ACCESSIBILITY_AUTOCLICK_DROPDOWN_DESCRIPTION },
    { "autoclickDelayExtremelyShort",
      IDS_OPTIONS_SETTINGS_ACCESSIBILITY_AUTOCLICK_DELAY_EXTREMELY_SHORT },
    { "autoclickDelayVeryShort",
      IDS_OPTIONS_SETTINGS_ACCESSIBILITY_AUTOCLICK_DELAY_VERY_SHORT },
    { "autoclickDelayShort",
      IDS_OPTIONS_SETTINGS_ACCESSIBILITY_AUTOCLICK_DELAY_SHORT },
    { "autoclickDelayLong",
      IDS_OPTIONS_SETTINGS_ACCESSIBILITY_AUTOCLICK_DELAY_LONG },
    { "autoclickDelayVeryLong",
      IDS_OPTIONS_SETTINGS_ACCESSIBILITY_AUTOCLICK_DELAY_VERY_LONG },
    { "enableContentProtectionAttestation",
      IDS_OPTIONS_ENABLE_CONTENT_PROTECTION_ATTESTATION },
    { "enableHotwordAppList", IDS_OPTIONS_ENABLE_HOTWORD_APP_LIST },
    { "factoryResetHeading", IDS_OPTIONS_FACTORY_RESET_HEADING },
    { "factoryResetTitle", IDS_OPTIONS_FACTORY_RESET },
    { "factoryResetRestart", IDS_OPTIONS_FACTORY_RESET_BUTTON },
    { "factoryResetDataRestart", IDS_RELAUNCH_BUTTON },
    { "factoryResetWarning", IDS_OPTIONS_FACTORY_RESET_WARNING },
    { "factoryResetHelpUrl", IDS_FACTORY_RESET_HELP_URL },
    { "changePicture", IDS_OPTIONS_CHANGE_PICTURE },
    { "changePictureCaption", IDS_OPTIONS_CHANGE_PICTURE_CAPTION },
    { "datetimeTitle", IDS_OPTIONS_SETTINGS_SECTION_TITLE_DATETIME },
    { "deviceGroupDescription", IDS_OPTIONS_DEVICE_GROUP_DESCRIPTION },
    { "deviceGroupPointer", IDS_OPTIONS_DEVICE_GROUP_POINTER_SECTION },
    { "mouseSpeed", IDS_OPTIONS_SETTINGS_MOUSE_SPEED_DESCRIPTION },
    { "touchpadSpeed", IDS_OPTIONS_SETTINGS_TOUCHPAD_SPEED_DESCRIPTION },
    { "enableScreenlock", IDS_OPTIONS_ENABLE_SCREENLOCKER_CHECKBOX },
    { "internetOptionsButtonTitle", IDS_OPTIONS_INTERNET_OPTIONS_BUTTON_TITLE },
    { "keyboardSettingsButtonTitle",
      IDS_OPTIONS_DEVICE_GROUP_KEYBOARD_SETTINGS_BUTTON_TITLE },
    { "manageAccountsButtonTitle", IDS_OPTIONS_ACCOUNTS_BUTTON_TITLE },
    { "noPointingDevices", IDS_OPTIONS_NO_POINTING_DEVICES },
    { "sectionTitleDevice", IDS_OPTIONS_DEVICE_GROUP_NAME },
    { "sectionTitleInternet", IDS_OPTIONS_INTERNET_OPTIONS_GROUP_LABEL },
    { "syncOverview", IDS_SYNC_OVERVIEW },
    { "syncButtonTextStart", IDS_SYNC_SETUP_BUTTON_LABEL },
    { "timezone", IDS_OPTIONS_SETTINGS_TIMEZONE_DESCRIPTION },
    { "use24HourClock", IDS_OPTIONS_SETTINGS_USE_24HOUR_CLOCK_DESCRIPTION },
#else
    { "cloudPrintManageButton",
      IDS_OPTIONS_CLOUD_PRINT_CONNECTOR_ENABLED_MANAGE_BUTTON},
    { "cloudPrintConnectorEnablingButton",
      IDS_OPTIONS_CLOUD_PRINT_CONNECTOR_ENABLING_BUTTON },
    { "proxiesConfigureButton", IDS_OPTIONS_PROXIES_CONFIGURE_BUTTON },
#endif
#if defined(OS_CHROMEOS) && defined(USE_ASH)
    { "setWallpaper", IDS_SET_WALLPAPER_BUTTON },
#endif
    { "advancedSectionTitleSystem",
      IDS_OPTIONS_ADVANCED_SECTION_TITLE_SYSTEM },
#if !defined(OS_MACOSX) && !defined(OS_CHROMEOS)
    { "backgroundModeCheckbox", IDS_OPTIONS_SYSTEM_ENABLE_BACKGROUND_MODE },
#endif
#if !defined(OS_CHROMEOS)
    { "gpuModeCheckbox",
      IDS_OPTIONS_SYSTEM_ENABLE_HARDWARE_ACCELERATION_MODE },
    { "gpuModeResetRestart",
      IDS_OPTIONS_SYSTEM_ENABLE_HARDWARE_ACCELERATION_MODE_RESTART },
    // Strings with product-name substitutions.
    { "syncOverview", IDS_SYNC_OVERVIEW, IDS_PRODUCT_NAME },
    { "syncButtonTextStart", IDS_SYNC_SETUP_BUTTON_LABEL },
#endif
    { "syncButtonTextSignIn", IDS_SYNC_START_SYNC_BUTTON_LABEL,
      IDS_SHORT_PRODUCT_NAME },
    { "profilesSingleUser", IDS_PROFILES_SINGLE_USER_MESSAGE,
      IDS_PRODUCT_NAME },
    { "defaultBrowserUnknown", IDS_OPTIONS_DEFAULTBROWSER_UNKNOWN,
      IDS_PRODUCT_NAME },
    { "defaultBrowserUseAsDefault", IDS_OPTIONS_DEFAULTBROWSER_USEASDEFAULT,
      IDS_PRODUCT_NAME },
    { "autoLaunchText", IDS_AUTOLAUNCH_TEXT, IDS_PRODUCT_NAME },
#if defined(OS_CHROMEOS)
    { "factoryResetDescription", IDS_OPTIONS_FACTORY_RESET_DESCRIPTION,
      IDS_SHORT_PRODUCT_NAME },
#endif
    { "languageSectionLabel", IDS_OPTIONS_ADVANCED_LANGUAGE_LABEL,
      IDS_SHORT_PRODUCT_NAME },
#if defined(ENABLE_MDNS)
    { "cloudPrintDevicesPageButton", IDS_LOCAL_DISCOVERY_DEVICES_PAGE_BUTTON },
    { "cloudPrintEnableNotificationsLabel",
      IDS_LOCAL_DISCOVERY_NOTIFICATIONS_ENABLE_CHECKBOX_LABEL },
#endif
  };

#if defined(ENABLE_SETTINGS_APP)
  static OptionsStringResource app_resources[] = {
    { "syncOverview", IDS_SETTINGS_APP_SYNC_OVERVIEW },
    { "syncButtonTextStart", IDS_SYNC_START_SYNC_BUTTON_LABEL,
      IDS_SETTINGS_APP_LAUNCHER_PRODUCT_NAME },
    { "profilesSingleUser", IDS_PROFILES_SINGLE_USER_MESSAGE,
      IDS_SETTINGS_APP_LAUNCHER_PRODUCT_NAME },
    { "languageSectionLabel", IDS_OPTIONS_ADVANCED_LANGUAGE_LABEL,
      IDS_SETTINGS_APP_LAUNCHER_PRODUCT_NAME },
    { "proxiesLabelSystem", IDS_OPTIONS_SYSTEM_PROXIES_LABEL,
      IDS_SETTINGS_APP_LAUNCHER_PRODUCT_NAME },
  };
  base::DictionaryValue* app_values = NULL;
  CHECK(values->GetDictionary(kSettingsAppKey, &app_values));
  RegisterStrings(app_values, app_resources, arraysize(app_resources));
#endif

  RegisterStrings(values, resources, arraysize(resources));
  RegisterTitle(values, "doNotTrackConfirmOverlay",
                IDS_OPTIONS_ENABLE_DO_NOT_TRACK_BUBBLE_TITLE);
  RegisterTitle(values, "spellingConfirmOverlay",
                IDS_CONTENT_CONTEXT_SPELLING_ASK_GOOGLE);
#if defined(ENABLE_FULL_PRINTING)
  RegisterCloudPrintValues(values);
#endif

  values->SetString("syncLearnMoreURL", chrome::kSyncLearnMoreURL);
  base::string16 omnibox_url = base::ASCIIToUTF16(chrome::kOmniboxLearnMoreURL);
  values->SetString(
      "defaultSearchGroupLabel",
      l10n_util::GetStringFUTF16(IDS_SEARCH_PREF_EXPLANATION, omnibox_url));
  values->SetString("hotwordLearnMoreURL", chrome::kHotwordLearnMoreURL);
  RegisterTitle(values, "hotwordConfirmOverlay",
                IDS_HOTWORD_SEARCH_PREF_CHKBOX);

#if defined(OS_CHROMEOS)
  Profile* profile = Profile::FromWebUI(web_ui());
  std::string username = profile->GetProfileName();
  if (username.empty()) {
    chromeos::User* user =
        chromeos::UserManager::Get()->GetUserByProfile(profile);
    if (user && (user->GetType() != chromeos::User::USER_TYPE_GUEST))
      username = user->email();

  }
  if (!username.empty())
    username = gaia::SanitizeEmail(gaia::CanonicalizeEmail(username));

  values->SetString("username", username);
#endif

  // Pass along sync status early so it will be available during page init.
  values->Set("syncData", GetSyncStateDictionary().release());

  // The Reset Profile Settings feature makes no sense for an off-the-record
  // profile (e.g. in Guest mode on Chrome OS), so hide it.
  values->SetBoolean("enableResetProfileSettings",
                     !Profile::FromWebUI(web_ui())->IsOffTheRecord());

  values->SetString("privacyLearnMoreURL", chrome::kPrivacyLearnMoreURL);
  values->SetString("doNotTrackLearnMoreURL", chrome::kDoNotTrackLearnMoreURL);

#if defined(OS_CHROMEOS)
  // TODO(pastarmovj): replace this with a call to the CrosSettings list
  // handling functionality to come.
  values->Set("timezoneList", chromeos::system::GetTimezoneList().release());

  values->SetString("accessibilityLearnMoreURL",
                    chrome::kChromeAccessibilityHelpURL);

  std::string settings_url = std::string("chrome-extension://") +
      extension_misc::kChromeVoxExtensionId +
      chrome::kChromeAccessibilitySettingsURL;

  values->SetString("accessibilitySettingsURL",
                    settings_url);

  values->SetString("contentProtectionAttestationLearnMoreURL",
                    chrome::kAttestationForContentProtectionLearnMoreURL);

  // Creates magnifierList.
  scoped_ptr<base::ListValue> magnifier_list(new base::ListValue);

  scoped_ptr<base::ListValue> option_full(new base::ListValue);
  option_full->AppendInteger(ash::MAGNIFIER_FULL);
  option_full->AppendString(l10n_util::GetStringUTF16(
      IDS_OPTIONS_SETTINGS_ACCESSIBILITY_SCREEN_MAGNIFIER_FULL));
  magnifier_list->Append(option_full.release());

  scoped_ptr<base::ListValue> option_partial(new base::ListValue);
  option_partial->AppendInteger(ash::MAGNIFIER_PARTIAL);
  option_partial->Append(new base::StringValue(l10n_util::GetStringUTF16(
      IDS_OPTIONS_SETTINGS_ACCESSIBILITY_SCREEN_MAGNIFIER_PARTIAL)));
  magnifier_list->Append(option_partial.release());

  values->Set("magnifierList", magnifier_list.release());

  scoped_ptr<base::FundamentalValue> should_show_app_list_hotword(
      new base::FundamentalValue(
          HotwordService::DoesHotwordSupportLanguage(profile)));
  values->Set(
      "shouldShowAppListHotword", should_show_app_list_hotword.release());
#endif

#if defined(OS_MACOSX)
  values->SetString("macPasswordsWarning",
      l10n_util::GetStringUTF16(IDS_OPTIONS_PASSWORDS_MAC_WARNING));
  values->SetBoolean("multiple_profiles",
      g_browser_process->profile_manager()->GetNumberOfProfiles() > 1);
#endif

  if (ShouldShowMultiProfilesUserList(helper::GetDesktopType(web_ui())))
    values->Set("profilesInfo", GetProfilesInfoList().release());

  values->SetBoolean("profileIsManaged",
                     Profile::FromWebUI(web_ui())->IsManaged());

#if !defined(OS_CHROMEOS)
  values->SetBoolean(
      "gpuEnabledAtStart",
      g_browser_process->gpu_mode_manager()->initial_gpu_mode_pref());
#endif

#if defined(ENABLE_MDNS)
  values->SetBoolean("cloudPrintHideNotificationsCheckbox",
                     !local_discovery::PrivetNotificationService::IsEnabled());
#endif

  values->SetBoolean("cloudPrintShowMDnsOptions",
                     cloud_print_mdns_ui_enabled_);

  values->SetString("cloudPrintLearnMoreURL", chrome::kCloudPrintLearnMoreURL);

  values->SetString("languagesLearnMoreURL",
                    chrome::kLanguageSettingsLearnMoreUrl);
}

#if defined(ENABLE_FULL_PRINTING)
void BrowserOptionsHandler::RegisterCloudPrintValues(
    base::DictionaryValue* values) {
  values->SetString("cloudPrintOptionLabel",
                    l10n_util::GetStringFUTF16(
                        IDS_CLOUD_PRINT_CHROMEOS_OPTION_LABEL,
                        l10n_util::GetStringUTF16(IDS_GOOGLE_CLOUD_PRINT)));

#if defined(OS_CHROMEOS)
  values->SetString("cloudPrintManageButton",
      l10n_util::GetStringFUTF16(
      IDS_CLOUD_PRINT_CHROMEOS_OPTION_BUTTON,
      l10n_util::GetStringUTF16(IDS_GOOGLE_CLOUD_PRINT)));
#else
  // TODO(noamsml): Remove all cloud print connector related code from the
  // settings page as soon as the devices page is supported on all platforms.
  values->SetString("cloudPrintConnectorDisabledLabel",
      l10n_util::GetStringFUTF16(
      IDS_OPTIONS_CLOUD_PRINT_CONNECTOR_DISABLED_LABEL,
      l10n_util::GetStringUTF16(IDS_GOOGLE_CLOUD_PRINT)));
  values->SetString("cloudPrintConnectorDisabledButton",
      l10n_util::GetStringUTF16(
      IDS_OPTIONS_CLOUD_PRINT_CONNECTOR_DISABLED_BUTTON));
  values->SetString("cloudPrintConnectorEnabledButton",
      l10n_util::GetStringUTF16(
      IDS_OPTIONS_CLOUD_PRINT_CONNECTOR_ENABLED_BUTTON));
#endif
}
#endif  // defined(ENABLE_FULL_PRINTING)

void BrowserOptionsHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "becomeDefaultBrowser",
      base::Bind(&BrowserOptionsHandler::BecomeDefaultBrowser,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setDefaultSearchEngine",
      base::Bind(&BrowserOptionsHandler::SetDefaultSearchEngine,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "deleteProfile",
      base::Bind(&BrowserOptionsHandler::DeleteProfile,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "themesReset",
      base::Bind(&BrowserOptionsHandler::ThemesReset,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "requestProfilesInfo",
      base::Bind(&BrowserOptionsHandler::HandleRequestProfilesInfo,
                 base::Unretained(this)));
#if defined(OS_LINUX) && !defined(OS_CHROMEOS)
  web_ui()->RegisterMessageCallback(
      "themesSetNative",
      base::Bind(&BrowserOptionsHandler::ThemesSetNative,
                 base::Unretained(this)));
#endif
  web_ui()->RegisterMessageCallback(
      "selectDownloadLocation",
      base::Bind(&BrowserOptionsHandler::HandleSelectDownloadLocation,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "autoOpenFileTypesAction",
      base::Bind(&BrowserOptionsHandler::HandleAutoOpenButton,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "defaultFontSizeAction",
      base::Bind(&BrowserOptionsHandler::HandleDefaultFontSize,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "defaultZoomFactorAction",
      base::Bind(&BrowserOptionsHandler::HandleDefaultZoomFactor,
                 base::Unretained(this)));
#if !defined(USE_NSS) && !defined(USE_OPENSSL)
  web_ui()->RegisterMessageCallback(
      "showManageSSLCertificates",
      base::Bind(&BrowserOptionsHandler::ShowManageSSLCertificates,
                 base::Unretained(this)));
#endif
#if defined(ENABLE_FULL_PRINTING)
  web_ui()->RegisterMessageCallback(
      "showCloudPrintManagePage",
      base::Bind(&BrowserOptionsHandler::ShowCloudPrintManagePage,
                 base::Unretained(this)));
#endif
#if defined(OS_CHROMEOS)
  web_ui()->RegisterMessageCallback(
      "openWallpaperManager",
      base::Bind(&BrowserOptionsHandler::HandleOpenWallpaperManager,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "virtualKeyboardChange",
      base::Bind(&BrowserOptionsHandler::VirtualKeyboardChangeCallback,
                 base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "performFactoryResetRestart",
      base::Bind(&BrowserOptionsHandler::PerformFactoryResetRestart,
                 base::Unretained(this)));
#else
  web_ui()->RegisterMessageCallback(
      "restartBrowser",
      base::Bind(&BrowserOptionsHandler::HandleRestartBrowser,
                 base::Unretained(this)));
#if defined(ENABLE_FULL_PRINTING)
  if (cloud_print_connector_ui_enabled_) {
    web_ui()->RegisterMessageCallback(
        "showCloudPrintSetupDialog",
        base::Bind(&BrowserOptionsHandler::ShowCloudPrintSetupDialog,
                   base::Unretained(this)));
    web_ui()->RegisterMessageCallback(
        "disableCloudPrintConnector",
        base::Bind(&BrowserOptionsHandler::HandleDisableCloudPrintConnector,
                   base::Unretained(this)));
  }
#endif  // defined(ENABLE_FULL_PRINTING)
  web_ui()->RegisterMessageCallback(
      "showNetworkProxySettings",
      base::Bind(&BrowserOptionsHandler::ShowNetworkProxySettings,
                 base::Unretained(this)));
#endif  // defined(OS_CHROMEOS)

#if defined(ENABLE_MDNS)
  if (cloud_print_mdns_ui_enabled_) {
    web_ui()->RegisterMessageCallback(
        "showCloudPrintDevicesPage",
        base::Bind(&BrowserOptionsHandler::ShowCloudPrintDevicesPage,
                   base::Unretained(this)));
  }
#endif
  web_ui()->RegisterMessageCallback(
      "requestHotwordAvailable",
      base::Bind(&BrowserOptionsHandler::HandleRequestHotwordAvailable,
                 base::Unretained(this)));
}

void BrowserOptionsHandler::Uninitialize() {
  registrar_.RemoveAll();
}

void BrowserOptionsHandler::OnStateChanged() {
  UpdateSyncState();
}

void BrowserOptionsHandler::PageLoadStarted() {
  page_initialized_ = false;
}

void BrowserOptionsHandler::InitializeHandler() {
  Profile* profile = Profile::FromWebUI(web_ui());
  PrefService* prefs = profile->GetPrefs();

  ProfileSyncService* sync_service(
      ProfileSyncServiceFactory::GetInstance()->GetForProfile(profile));
  if (sync_service)
    sync_service->AddObserver(this);

  // Create our favicon data source.
  content::URLDataSource::Add(
      profile, new FaviconSource(profile, FaviconSource::FAVICON));

  default_browser_policy_.Init(
      prefs::kDefaultBrowserSettingEnabled,
      g_browser_process->local_state(),
      base::Bind(&BrowserOptionsHandler::UpdateDefaultBrowserState,
                 base::Unretained(this)));

  registrar_.Add(this, chrome::NOTIFICATION_PROFILE_CACHED_INFO_CHANGED,
                 content::NotificationService::AllSources());
#if defined(OS_CHROMEOS)
  registrar_.Add(this, chrome::NOTIFICATION_LOGIN_USER_IMAGE_CHANGED,
                 content::NotificationService::AllSources());
#endif
  registrar_.Add(this, chrome::NOTIFICATION_BROWSER_THEME_CHANGED,
                 content::Source<ThemeService>(
                     ThemeServiceFactory::GetForProfile(profile)));
  registrar_.Add(this, chrome::NOTIFICATION_GLOBAL_ERRORS_CHANGED,
                 content::Source<Profile>(profile));
  registrar_.Add(this, chrome::NOTIFICATION_GOOGLE_SIGNIN_SUCCESSFUL,
                 content::Source<Profile>(profile));
  registrar_.Add(this, chrome::NOTIFICATION_GOOGLE_SIGNED_OUT,
                 content::Source<Profile>(profile));
  AddTemplateUrlServiceObserver();

#if defined(OS_WIN)
  const CommandLine& command_line = *CommandLine::ForCurrentProcess();
  if (!command_line.HasSwitch(switches::kUserDataDir)) {
    BrowserThread::PostTask(BrowserThread::FILE, FROM_HERE,
        base::Bind(&BrowserOptionsHandler::CheckAutoLaunch,
                   weak_ptr_factory_.GetWeakPtr(),
                   profile->GetPath()));
  }
#endif

#if defined(ENABLE_FULL_PRINTING) && !defined(OS_CHROMEOS)
  base::Closure cloud_print_callback = base::Bind(
      &BrowserOptionsHandler::OnCloudPrintPrefsChanged, base::Unretained(this));
  cloud_print_connector_email_.Init(
      prefs::kCloudPrintEmail, prefs, cloud_print_callback);
  cloud_print_connector_enabled_.Init(
      prefs::kCloudPrintProxyEnabled, prefs, cloud_print_callback);
#endif

  auto_open_files_.Init(
      prefs::kDownloadExtensionsToOpen, prefs,
      base::Bind(&BrowserOptionsHandler::SetupAutoOpenFileTypes,
                 base::Unretained(this)));
  default_zoom_level_.Init(
      prefs::kDefaultZoomLevel, prefs,
      base::Bind(&BrowserOptionsHandler::SetupPageZoomSelector,
                 base::Unretained(this)));
  profile_pref_registrar_.Init(prefs);
  profile_pref_registrar_.Add(
      prefs::kWebKitDefaultFontSize,
      base::Bind(&BrowserOptionsHandler::SetupFontSizeSelector,
                 base::Unretained(this)));
  profile_pref_registrar_.Add(
      prefs::kWebKitDefaultFixedFontSize,
      base::Bind(&BrowserOptionsHandler::SetupFontSizeSelector,
                 base::Unretained(this)));
  profile_pref_registrar_.Add(
      prefs::kManagedUsers,
      base::Bind(&BrowserOptionsHandler::SetupManagingSupervisedUsers,
                 base::Unretained(this)));
  profile_pref_registrar_.Add(
      prefs::kSigninAllowed,
      base::Bind(&BrowserOptionsHandler::OnSigninAllowedPrefChange,
                 base::Unretained(this)));

#if defined(OS_CHROMEOS)
  if (!policy_registrar_) {
    policy_registrar_.reset(new policy::PolicyChangeRegistrar(
        policy::ProfilePolicyConnectorFactory::GetForProfile(profile)->
            policy_service(),
        policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, std::string())));
    policy_registrar_->Observe(
        policy::key::kUserAvatarImage,
        base::Bind(&BrowserOptionsHandler::OnUserImagePolicyChanged,
                   base::Unretained(this)));
  }
#else  // !defined(OS_CHROMEOS)
  profile_pref_registrar_.Add(
      prefs::kProxy,
      base::Bind(&BrowserOptionsHandler::SetupProxySettingsSection,
                 base::Unretained(this)));
#endif  // !defined(OS_CHROMEOS)
}

void BrowserOptionsHandler::InitializePage() {
  page_initialized_ = true;

  OnTemplateURLServiceChanged();

  ObserveThemeChanged();
  OnStateChanged();
  UpdateDefaultBrowserState();

  SetupMetricsReportingSettingVisibility();
  SetupFontSizeSelector();
  SetupPageZoomSelector();
  SetupAutoOpenFileTypes();
  SetupProxySettingsSection();
  SetupManageCertificatesSection();
  SetupManagingSupervisedUsers();

#if defined(ENABLE_FULL_PRINTING) && !defined(OS_CHROMEOS)
  if (!cloud_print_mdns_ui_enabled_) {
    if (cloud_print_connector_ui_enabled_) {
      SetupCloudPrintConnectorSection();
      RefreshCloudPrintStatusFromService();
    } else {
      RemoveCloudPrintConnectorSection();
    }
  }
#endif

#if defined(OS_CHROMEOS)
  SetupAccessibilityFeatures();
  policy::BrowserPolicyConnectorChromeOS* connector =
      g_browser_process->platform_part()->browser_policy_connector_chromeos();
  if (!connector->IsEnterpriseManaged() &&
      !chromeos::UserManager::Get()->IsLoggedInAsGuest() &&
      !chromeos::UserManager::Get()->IsLoggedInAsLocallyManagedUser()) {
    web_ui()->CallJavascriptFunction(
        "BrowserOptions.enableFactoryResetSection");
  }

  Profile* profile = Profile::FromWebUI(web_ui());
  OnAccountPictureManagedChanged(
      policy::ProfilePolicyConnectorFactory::GetForProfile(profile)->
          policy_service()->GetPolicies(
              policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME,
                                      std::string()))
             .Get(policy::key::kUserAvatarImage));
#endif
}

// static
void BrowserOptionsHandler::CheckAutoLaunch(
    base::WeakPtr<BrowserOptionsHandler> weak_this,
    const base::FilePath& profile_path) {
#if defined(OS_WIN)
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));

  // Auto-launch is not supported for secondary profiles yet.
  if (profile_path.BaseName().value() !=
          base::ASCIIToUTF16(chrome::kInitialProfile)) {
    return;
  }

  // Pass in weak pointer to this to avoid race if BrowserOptionsHandler is
  // deleted.
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
      base::Bind(&BrowserOptionsHandler::CheckAutoLaunchCallback,
                 weak_this,
                 auto_launch_trial::IsInAutoLaunchGroup(),
                 auto_launch_util::AutoStartRequested(
                     profile_path.BaseName().value(),
                     true,  // Window requested.
                     base::FilePath())));
#endif
}

void BrowserOptionsHandler::CheckAutoLaunchCallback(
    bool is_in_auto_launch_group,
    bool will_launch_at_login) {
#if defined(OS_WIN)
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  if (is_in_auto_launch_group) {
    web_ui()->RegisterMessageCallback("toggleAutoLaunch",
        base::Bind(&BrowserOptionsHandler::ToggleAutoLaunch,
        base::Unretained(this)));

    base::FundamentalValue enabled(will_launch_at_login);
    web_ui()->CallJavascriptFunction("BrowserOptions.updateAutoLaunchState",
                                     enabled);
  }
#endif
}

void BrowserOptionsHandler::UpdateDefaultBrowserState() {
#if defined(OS_MACOSX)
  ShellIntegration::DefaultWebClientState state =
      ShellIntegration::GetDefaultBrowser();
  int status_string_id;
  if (state == ShellIntegration::IS_DEFAULT)
    status_string_id = IDS_OPTIONS_DEFAULTBROWSER_DEFAULT;
  else if (state == ShellIntegration::NOT_DEFAULT)
    status_string_id = IDS_OPTIONS_DEFAULTBROWSER_NOTDEFAULT;
  else
    status_string_id = IDS_OPTIONS_DEFAULTBROWSER_UNKNOWN;

  SetDefaultBrowserUIString(status_string_id);
#else
  default_browser_worker_->StartCheckIsDefault();
#endif
}

void BrowserOptionsHandler::BecomeDefaultBrowser(const base::ListValue* args) {
  // If the default browser setting is managed then we should not be able to
  // call this function.
  if (default_browser_policy_.IsManaged())
    return;

  content::RecordAction(UserMetricsAction("Options_SetAsDefaultBrowser"));
#if defined(OS_MACOSX)
  if (ShellIntegration::SetAsDefaultBrowser())
    UpdateDefaultBrowserState();
#else
  default_browser_worker_->StartSetAsDefault();
  // Callback takes care of updating UI.
#endif

  // If the user attempted to make Chrome the default browser, then he/she
  // arguably wants to be notified when that changes.
  PrefService* prefs = Profile::FromWebUI(web_ui())->GetPrefs();
  prefs->SetBoolean(prefs::kCheckDefaultBrowser, true);
}

int BrowserOptionsHandler::StatusStringIdForState(
    ShellIntegration::DefaultWebClientState state) {
  if (state == ShellIntegration::IS_DEFAULT)
    return IDS_OPTIONS_DEFAULTBROWSER_DEFAULT;
  if (state == ShellIntegration::NOT_DEFAULT)
    return IDS_OPTIONS_DEFAULTBROWSER_NOTDEFAULT;
  return IDS_OPTIONS_DEFAULTBROWSER_UNKNOWN;
}

void BrowserOptionsHandler::SetDefaultWebClientUIState(
    ShellIntegration::DefaultWebClientUIState state) {
  int status_string_id;

  if (state == ShellIntegration::STATE_IS_DEFAULT) {
    status_string_id = IDS_OPTIONS_DEFAULTBROWSER_DEFAULT;
  } else if (state == ShellIntegration::STATE_NOT_DEFAULT) {
    if (ShellIntegration::CanSetAsDefaultBrowser() ==
            ShellIntegration::SET_DEFAULT_NOT_ALLOWED) {
      status_string_id = IDS_OPTIONS_DEFAULTBROWSER_SXS;
    } else {
      status_string_id = IDS_OPTIONS_DEFAULTBROWSER_NOTDEFAULT;
    }
  } else if (state == ShellIntegration::STATE_UNKNOWN) {
    status_string_id = IDS_OPTIONS_DEFAULTBROWSER_UNKNOWN;
  } else {
    return;  // Still processing.
  }

  SetDefaultBrowserUIString(status_string_id);
}

bool BrowserOptionsHandler::IsInteractiveSetDefaultPermitted() {
  return true;  // This is UI so we can allow it.
}

void BrowserOptionsHandler::SetDefaultBrowserUIString(int status_string_id) {
  base::StringValue status_string(
      l10n_util::GetStringFUTF16(status_string_id,
                                 l10n_util::GetStringUTF16(IDS_PRODUCT_NAME)));

  base::FundamentalValue is_default(
      status_string_id == IDS_OPTIONS_DEFAULTBROWSER_DEFAULT);

  base::FundamentalValue can_be_default(
      !default_browser_policy_.IsManaged() &&
      (status_string_id == IDS_OPTIONS_DEFAULTBROWSER_DEFAULT ||
       status_string_id == IDS_OPTIONS_DEFAULTBROWSER_NOTDEFAULT));

  web_ui()->CallJavascriptFunction(
      "BrowserOptions.updateDefaultBrowserState",
      status_string, is_default, can_be_default);
}

void BrowserOptionsHandler::OnTemplateURLServiceChanged() {
  if (!template_url_service_ || !template_url_service_->loaded())
    return;

  const TemplateURL* default_url =
      template_url_service_->GetDefaultSearchProvider();

  int default_index = -1;
  base::ListValue search_engines;
  TemplateURLService::TemplateURLVector model_urls(
      template_url_service_->GetTemplateURLs());
  for (size_t i = 0; i < model_urls.size(); ++i) {
    if (!model_urls[i]->ShowInDefaultList())
      continue;

    base::DictionaryValue* entry = new base::DictionaryValue();
    entry->SetString("name", model_urls[i]->short_name());
    entry->SetInteger("index", i);
    search_engines.Append(entry);
    if (model_urls[i] == default_url)
      default_index = i;
  }

  web_ui()->CallJavascriptFunction(
      "BrowserOptions.updateSearchEngines",
      search_engines,
      base::FundamentalValue(default_index),
      base::FundamentalValue(
          template_url_service_->is_default_search_managed() ||
          template_url_service_->IsExtensionControlledDefaultSearch()));
}

void BrowserOptionsHandler::SetDefaultSearchEngine(
    const base::ListValue* args) {
  int selected_index = -1;
  if (!ExtractIntegerValue(args, &selected_index)) {
    NOTREACHED();
    return;
  }

  TemplateURLService::TemplateURLVector model_urls(
      template_url_service_->GetTemplateURLs());
  if (selected_index >= 0 &&
      selected_index < static_cast<int>(model_urls.size()))
    template_url_service_->SetDefaultSearchProvider(model_urls[selected_index]);

  content::RecordAction(UserMetricsAction("Options_SearchEngineChanged"));
}

void BrowserOptionsHandler::AddTemplateUrlServiceObserver() {
  template_url_service_ =
      TemplateURLServiceFactory::GetForProfile(Profile::FromWebUI(web_ui()));
  if (template_url_service_) {
    template_url_service_->Load();
    template_url_service_->AddObserver(this);
  }
}

void BrowserOptionsHandler::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  // Notifications are used to update the UI dynamically when settings change in
  // the background. If the UI is currently being loaded, no dynamic updates are
  // possible (as the DOM and JS are not fully loaded) or necessary (as
  // InitializePage() will update the UI at the end of the load).
  if (!page_initialized_)
    return;

  switch (type) {
    case chrome::NOTIFICATION_BROWSER_THEME_CHANGED:
      ObserveThemeChanged();
      break;
#if defined(OS_CHROMEOS)
    case chrome::NOTIFICATION_LOGIN_USER_IMAGE_CHANGED:
      UpdateAccountPicture();
      break;
#endif
    case chrome::NOTIFICATION_PROFILE_CACHED_INFO_CHANGED:
    SendProfilesInfo();
      break;
    case chrome::NOTIFICATION_GLOBAL_ERRORS_CHANGED:
    case chrome::NOTIFICATION_GOOGLE_SIGNIN_SUCCESSFUL:
    case chrome::NOTIFICATION_GOOGLE_SIGNED_OUT:
      // Update our sync/signin status display.
      OnStateChanged();
      break;
    default:
      NOTREACHED();
  }
}

#if defined(ENABLE_FULL_PRINTING) && !defined(OS_CHROMEOS)
void BrowserOptionsHandler::OnCloudPrintPrefsChanged() {
  if (cloud_print_connector_ui_enabled_)
    SetupCloudPrintConnectorSection();
}
#endif

void BrowserOptionsHandler::ToggleAutoLaunch(const base::ListValue* args) {
#if defined(OS_WIN)
  if (!auto_launch_trial::IsInAutoLaunchGroup())
    return;

  bool enable;
  CHECK_EQ(args->GetSize(), 1U);
  CHECK(args->GetBoolean(0, &enable));

  Profile* profile = Profile::FromWebUI(web_ui());
  content::BrowserThread::PostTask(
      content::BrowserThread::FILE, FROM_HERE,
      enable ?
          base::Bind(&auto_launch_util::EnableForegroundStartAtLogin,
                     profile->GetPath().BaseName().value(), base::FilePath()) :
          base::Bind(&auto_launch_util::DisableForegroundStartAtLogin,
                      profile->GetPath().BaseName().value()));
#endif  // OS_WIN
}

scoped_ptr<base::ListValue> BrowserOptionsHandler::GetProfilesInfoList() {
  ProfileInfoCache& cache =
      g_browser_process->profile_manager()->GetProfileInfoCache();
  scoped_ptr<base::ListValue> profile_info_list(new base::ListValue);
  base::FilePath current_profile_path =
      web_ui()->GetWebContents()->GetBrowserContext()->GetPath();

  for (size_t i = 0, e = cache.GetNumberOfProfiles(); i < e; ++i) {
    base::DictionaryValue* profile_value = new base::DictionaryValue();
    profile_value->SetString("name", cache.GetNameOfProfileAtIndex(i));
    base::FilePath profile_path = cache.GetPathOfProfileAtIndex(i);
    profile_value->Set("filePath", base::CreateFilePathValue(profile_path));
    profile_value->SetBoolean("isCurrentProfile",
                              profile_path == current_profile_path);
    profile_value->SetBoolean("isManaged", cache.ProfileIsManagedAtIndex(i));

    bool is_gaia_picture =
        cache.IsUsingGAIAPictureOfProfileAtIndex(i) &&
        cache.GetGAIAPictureOfProfileAtIndex(i);
    if (is_gaia_picture) {
      gfx::Image icon = profiles::GetAvatarIconForWebUI(
          cache.GetAvatarIconOfProfileAtIndex(i), true);
      profile_value->SetString("iconURL",
          webui::GetBitmapDataUrl(icon.AsBitmap()));
    } else {
      size_t icon_index = cache.GetAvatarIconIndexOfProfileAtIndex(i);
      profile_value->SetString("iconURL",
                               cache.GetDefaultAvatarIconUrl(icon_index));
    }

    profile_info_list->Append(profile_value);
  }

  return profile_info_list.Pass();
}

void BrowserOptionsHandler::SendProfilesInfo() {
  if (!ShouldShowMultiProfilesUserList(helper::GetDesktopType(web_ui())))
    return;
  web_ui()->CallJavascriptFunction("BrowserOptions.setProfilesInfo",
                                   *GetProfilesInfoList());
}

void BrowserOptionsHandler::DeleteProfile(const base::ListValue* args) {
  DCHECK(args);
  const base::Value* file_path_value;
  if (!args->Get(0, &file_path_value))
    return;

  base::FilePath file_path;
  if (!base::GetValueAsFilePath(*file_path_value, &file_path))
    return;
  helper::DeleteProfileAtPath(file_path, web_ui());
}

void BrowserOptionsHandler::ObserveThemeChanged() {
  Profile* profile = Profile::FromWebUI(web_ui());
  ThemeService* theme_service = ThemeServiceFactory::GetForProfile(profile);
  bool is_native_theme = false;

#if defined(OS_LINUX) && !defined(OS_CHROMEOS)
  bool profile_is_managed = profile->IsManaged();
  is_native_theme = theme_service->UsingNativeTheme();
  base::FundamentalValue native_theme_enabled(!is_native_theme &&
                                              !profile_is_managed);
  web_ui()->CallJavascriptFunction("BrowserOptions.setNativeThemeButtonEnabled",
                                   native_theme_enabled);
#endif

  bool is_classic_theme = !is_native_theme &&
                          theme_service->UsingDefaultTheme();
  base::FundamentalValue enabled(!is_classic_theme);
  web_ui()->CallJavascriptFunction("BrowserOptions.setThemesResetButtonEnabled",
                                   enabled);
}

void BrowserOptionsHandler::ThemesReset(const base::ListValue* args) {
  Profile* profile = Profile::FromWebUI(web_ui());
  content::RecordAction(UserMetricsAction("Options_ThemesReset"));
  ThemeServiceFactory::GetForProfile(profile)->UseDefaultTheme();
}

#if defined(OS_LINUX) && !defined(OS_CHROMEOS)
void BrowserOptionsHandler::ThemesSetNative(const base::ListValue* args) {
  content::RecordAction(UserMetricsAction("Options_GtkThemeSet"));
  Profile* profile = Profile::FromWebUI(web_ui());
  ThemeServiceFactory::GetForProfile(profile)->SetNativeTheme();
}
#endif

#if defined(OS_CHROMEOS)
void BrowserOptionsHandler::UpdateAccountPicture() {
  std::string email = chromeos::UserManager::Get()->GetLoggedInUser()->email();
  if (!email.empty()) {
    web_ui()->CallJavascriptFunction("BrowserOptions.updateAccountPicture");
    base::StringValue email_value(email);
    web_ui()->CallJavascriptFunction("BrowserOptions.updateAccountPicture",
                                     email_value);
  }
}

void BrowserOptionsHandler::OnAccountPictureManagedChanged(bool managed) {
  web_ui()->CallJavascriptFunction("BrowserOptions.setAccountPictureManaged",
                                   base::FundamentalValue(managed));
}
#endif

scoped_ptr<base::DictionaryValue>
BrowserOptionsHandler::GetSyncStateDictionary() {
  scoped_ptr<base::DictionaryValue> sync_status(new base::DictionaryValue);
  Profile* profile = Profile::FromWebUI(web_ui());
  if (profile->IsManaged()) {
    sync_status->SetBoolean("supervisedUser", true);
    sync_status->SetBoolean("signinAllowed", false);
    return sync_status.Pass();
  }
  if (profile->IsGuestSession()) {
    // Cannot display signin status when running in guest mode on chromeos
    // because there is no SigninManager.
    sync_status->SetBoolean("signinAllowed", false);
    return sync_status.Pass();
  }
  sync_status->SetBoolean("supervisedUser", false);

  bool signout_prohibited = false;
#if !defined(OS_CHROMEOS)
  // Signout is not allowed if the user has policy (crbug.com/172204).
  signout_prohibited =
      SigninManagerFactory::GetForProfile(profile)->IsSignoutProhibited();
#endif

  ProfileSyncService* service =
      ProfileSyncServiceFactory::GetInstance()->GetForProfile(profile);
  SigninManagerBase* signin = SigninManagerFactory::GetForProfile(profile);
  DCHECK(signin);
  sync_status->SetBoolean("signoutAllowed", !signout_prohibited);
  sync_status->SetBoolean("signinAllowed", signin->IsSigninAllowed());
  sync_status->SetBoolean("syncSystemEnabled", !!service);
  sync_status->SetBoolean("setupCompleted",
                          service && service->HasSyncSetupCompleted());
  sync_status->SetBoolean("setupInProgress",
      service && !service->IsManaged() && service->FirstSetupInProgress());

  base::string16 status_label;
  base::string16 link_label;
  bool status_has_error = sync_ui_util::GetStatusLabels(
      service, *signin, sync_ui_util::WITH_HTML, &status_label, &link_label) ==
          sync_ui_util::SYNC_ERROR;
  sync_status->SetString("statusText", status_label);
  sync_status->SetString("actionLinkText", link_label);
  sync_status->SetBoolean("hasError", status_has_error);

  sync_status->SetBoolean("managed", service && service->IsManaged());
  sync_status->SetBoolean("signedIn",
                          !signin->GetAuthenticatedUsername().empty());
  sync_status->SetBoolean("hasUnrecoverableError",
                          service && service->HasUnrecoverableError());
  sync_status->SetBoolean(
      "autoLoginVisible",
      CommandLine::ForCurrentProcess()->HasSwitch(switches::kEnableAutologin) &&
      service && service->IsSyncEnabledAndLoggedIn() &&
      service->IsOAuthRefreshTokenAvailable());

  return sync_status.Pass();
}

void BrowserOptionsHandler::HandleSelectDownloadLocation(
    const base::ListValue* args) {
  PrefService* pref_service = Profile::FromWebUI(web_ui())->GetPrefs();
  select_folder_dialog_ = ui::SelectFileDialog::Create(
      this, new ChromeSelectFilePolicy(web_ui()->GetWebContents()));
  ui::SelectFileDialog::FileTypeInfo info;
  info.support_drive = true;
  select_folder_dialog_->SelectFile(
      ui::SelectFileDialog::SELECT_FOLDER,
      l10n_util::GetStringUTF16(IDS_OPTIONS_DOWNLOADLOCATION_BROWSE_TITLE),
      pref_service->GetFilePath(prefs::kDownloadDefaultDirectory),
      &info,
      0,
      base::FilePath::StringType(),
      web_ui()->GetWebContents()->GetView()->GetTopLevelNativeWindow(),
      NULL);
}

void BrowserOptionsHandler::FileSelected(const base::FilePath& path, int index,
                                         void* params) {
  content::RecordAction(UserMetricsAction("Options_SetDownloadDirectory"));
  PrefService* pref_service = Profile::FromWebUI(web_ui())->GetPrefs();
  pref_service->SetFilePath(prefs::kDownloadDefaultDirectory, path);
  pref_service->SetFilePath(prefs::kSaveFileDefaultDirectory, path);
}

#if defined(OS_CHROMEOS)
void BrowserOptionsHandler::TouchpadExists(bool exists) {
  base::FundamentalValue val(exists);
  web_ui()->CallJavascriptFunction("BrowserOptions.showTouchpadControls", val);
}

void BrowserOptionsHandler::MouseExists(bool exists) {
  base::FundamentalValue val(exists);
  web_ui()->CallJavascriptFunction("BrowserOptions.showMouseControls", val);
}

void BrowserOptionsHandler::OnUserImagePolicyChanged(
    const base::Value* previous_policy,
    const base::Value* current_policy) {
  const bool had_policy = !!previous_policy;
  const bool has_policy = !!current_policy;
  if (had_policy != has_policy)
    OnAccountPictureManagedChanged(has_policy);
}

#endif  // defined(OS_CHROMEOS)

void BrowserOptionsHandler::UpdateSyncState() {
  web_ui()->CallJavascriptFunction("BrowserOptions.updateSyncState",
                                   *GetSyncStateDictionary());
}

void BrowserOptionsHandler::OnSigninAllowedPrefChange() {
  UpdateSyncState();
}

void BrowserOptionsHandler::HandleAutoOpenButton(const base::ListValue* args) {
  content::RecordAction(UserMetricsAction("Options_ResetAutoOpenFiles"));
  DownloadManager* manager = BrowserContext::GetDownloadManager(
      web_ui()->GetWebContents()->GetBrowserContext());
  if (manager)
    DownloadPrefs::FromDownloadManager(manager)->ResetAutoOpen();
}

void BrowserOptionsHandler::HandleDefaultFontSize(const base::ListValue* args) {
  int font_size;
  if (ExtractIntegerValue(args, &font_size)) {
    if (font_size > 0) {
      PrefService* pref_service = Profile::FromWebUI(web_ui())->GetPrefs();
      pref_service->SetInteger(prefs::kWebKitDefaultFontSize, font_size);
      SetupFontSizeSelector();
    }
  }
}

void BrowserOptionsHandler::HandleDefaultZoomFactor(
    const base::ListValue* args) {
  double zoom_factor;
  if (ExtractDoubleValue(args, &zoom_factor)) {
    default_zoom_level_.SetValue(content::ZoomFactorToZoomLevel(zoom_factor));
  }
}

void BrowserOptionsHandler::HandleRestartBrowser(const base::ListValue* args) {
#if defined(OS_WIN) && defined(USE_ASH)
  // If hardware acceleration is disabled then we need to force restart
  // browser in desktop mode.
  // TODO(shrikant): Remove this once we fix start mode logic for browser.
  // Currently there are issues with determining correct browser mode
  // at startup.
  if (chrome::GetActiveDesktop() == chrome::HOST_DESKTOP_TYPE_ASH) {
    PrefService* pref_service = g_browser_process->local_state();
    if (!pref_service->GetBoolean(prefs::kHardwareAccelerationModeEnabled)) {
      chrome::AttemptRestartToDesktopMode();
      return;
    }
  }
#endif

  chrome::AttemptRestart();
}

void BrowserOptionsHandler::HandleRequestProfilesInfo(
    const base::ListValue* args) {
  SendProfilesInfo();
}

#if !defined(OS_CHROMEOS)
void BrowserOptionsHandler::ShowNetworkProxySettings(
    const base::ListValue* args) {
  content::RecordAction(UserMetricsAction("Options_ShowProxySettings"));
  AdvancedOptionsUtilities::ShowNetworkProxySettings(
      web_ui()->GetWebContents());
}
#endif

#if !defined(USE_NSS) && !defined(USE_OPENSSL)
void BrowserOptionsHandler::ShowManageSSLCertificates(
    const base::ListValue* args) {
  content::RecordAction(UserMetricsAction("Options_ManageSSLCertificates"));
  AdvancedOptionsUtilities::ShowManageSSLCertificates(
      web_ui()->GetWebContents());
}
#endif

#if defined(ENABLE_MDNS)

void BrowserOptionsHandler::ShowCloudPrintDevicesPage(
    const base::ListValue* args) {
  content::RecordAction(UserMetricsAction("Options_CloudPrintDevicesPage"));
  // Navigate in current tab to devices page.
  OpenURLParams params(
      GURL(chrome::kChromeUIDevicesURL), Referrer(),
      CURRENT_TAB, content::PAGE_TRANSITION_LINK, false);
  web_ui()->GetWebContents()->OpenURL(params);
}

#endif

#if defined(ENABLE_FULL_PRINTING)
void BrowserOptionsHandler::ShowCloudPrintManagePage(
    const base::ListValue* args) {
  content::RecordAction(UserMetricsAction("Options_ManageCloudPrinters"));
  // Open a new tab in the current window for the management page.
  Profile* profile = Profile::FromWebUI(web_ui());
  OpenURLParams params(
      CloudPrintURL(profile).GetCloudPrintServiceManageURL(), Referrer(),
      NEW_FOREGROUND_TAB, content::PAGE_TRANSITION_LINK, false);
  web_ui()->GetWebContents()->OpenURL(params);
}

#if !defined(OS_CHROMEOS)
void BrowserOptionsHandler::ShowCloudPrintSetupDialog(
    const base::ListValue* args) {
  content::RecordAction(UserMetricsAction("Options_EnableCloudPrintProxy"));
  // Open the connector enable page in the current tab.
  Profile* profile = Profile::FromWebUI(web_ui());
  OpenURLParams params(
      CloudPrintURL(profile).GetCloudPrintServiceEnableURL(
          CloudPrintProxyServiceFactory::GetForProfile(profile)->proxy_id()),
      Referrer(), CURRENT_TAB, content::PAGE_TRANSITION_LINK, false);
  web_ui()->GetWebContents()->OpenURL(params);
}

void BrowserOptionsHandler::HandleDisableCloudPrintConnector(
    const base::ListValue* args) {
  content::RecordAction(
      UserMetricsAction("Options_DisableCloudPrintProxy"));
  CloudPrintProxyServiceFactory::GetForProfile(Profile::FromWebUI(web_ui()))->
      DisableForUser();
}

void BrowserOptionsHandler::RefreshCloudPrintStatusFromService() {
  if (cloud_print_connector_ui_enabled_)
    CloudPrintProxyServiceFactory::GetForProfile(Profile::FromWebUI(web_ui()))->
        RefreshStatusFromService();
}

void BrowserOptionsHandler::SetupCloudPrintConnectorSection() {
  Profile* profile = Profile::FromWebUI(web_ui());
  if (!CloudPrintProxyServiceFactory::GetForProfile(profile)) {
    cloud_print_connector_ui_enabled_ = false;
    RemoveCloudPrintConnectorSection();
    return;
  }

  bool cloud_print_connector_allowed =
      !cloud_print_connector_enabled_.IsManaged() ||
      cloud_print_connector_enabled_.GetValue();
  base::FundamentalValue allowed(cloud_print_connector_allowed);

  std::string email;
  if (profile->GetPrefs()->HasPrefPath(prefs::kCloudPrintEmail) &&
      cloud_print_connector_allowed) {
    email = profile->GetPrefs()->GetString(prefs::kCloudPrintEmail);
  }
  base::FundamentalValue disabled(email.empty());

  base::string16 label_str;
  if (email.empty()) {
    label_str = l10n_util::GetStringFUTF16(
        IDS_OPTIONS_CLOUD_PRINT_CONNECTOR_DISABLED_LABEL,
        l10n_util::GetStringUTF16(IDS_GOOGLE_CLOUD_PRINT));
  } else {
    label_str = l10n_util::GetStringFUTF16(
        IDS_OPTIONS_CLOUD_PRINT_CONNECTOR_ENABLED_LABEL,
        l10n_util::GetStringUTF16(IDS_GOOGLE_CLOUD_PRINT),
        base::UTF8ToUTF16(email));
  }
  base::StringValue label(label_str);

  web_ui()->CallJavascriptFunction(
      "BrowserOptions.setupCloudPrintConnectorSection", disabled, label,
      allowed);
}

void BrowserOptionsHandler::RemoveCloudPrintConnectorSection() {
  web_ui()->CallJavascriptFunction(
      "BrowserOptions.removeCloudPrintConnectorSection");
}
#endif  // defined(OS_CHROMEOS)
#endif  // defined(ENABLE_FULL_PRINTING)

void BrowserOptionsHandler::HandleRequestHotwordAvailable(
    const base::ListValue* args) {
  Profile* profile = Profile::FromWebUI(web_ui());
  std::string group = base::FieldTrialList::FindFullName("VoiceTrigger");
  if (group != "" && group != "Disabled" &&
      HotwordServiceFactory::IsServiceAvailable(profile)) {
    web_ui()->CallJavascriptFunction("BrowserOptions.showHotwordSection");
  }
}

#if defined(OS_CHROMEOS)
void BrowserOptionsHandler::HandleOpenWallpaperManager(
    const base::ListValue* args) {
  wallpaper_manager_util::OpenWallpaperManager();
}

void BrowserOptionsHandler::VirtualKeyboardChangeCallback(
    const base::ListValue* args) {
  bool enabled = false;
  args->GetBoolean(0, &enabled);

  chromeos::accessibility::EnableVirtualKeyboard(enabled);
}

void BrowserOptionsHandler::PerformFactoryResetRestart(
    const base::ListValue* args) {
  policy::BrowserPolicyConnectorChromeOS* connector =
      g_browser_process->platform_part()->browser_policy_connector_chromeos();
  if (connector->IsEnterpriseManaged())
    return;

  PrefService* prefs = g_browser_process->local_state();
  prefs->SetBoolean(prefs::kFactoryResetRequested, true);
  prefs->CommitPendingWrite();

  // Perform sign out. Current chrome process will then terminate, new one will
  // be launched (as if it was a restart).
  chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->RequestRestart();
}

void BrowserOptionsHandler::SetupAccessibilityFeatures() {
  PrefService* pref_service = g_browser_process->local_state();
  base::FundamentalValue virtual_keyboard_enabled(
      pref_service->GetBoolean(prefs::kVirtualKeyboardEnabled));
  web_ui()->CallJavascriptFunction(
      "BrowserOptions.setVirtualKeyboardCheckboxState",
      virtual_keyboard_enabled);
}
#endif

void BrowserOptionsHandler::SetupMetricsReportingSettingVisibility() {
#if defined(GOOGLE_CHROME_BUILD) && defined(OS_CHROMEOS)
  // Don't show the reporting setting if we are in the guest mode.
  if (CommandLine::ForCurrentProcess()->HasSwitch(
          chromeos::switches::kGuestSession)) {
    base::FundamentalValue visible(false);
    web_ui()->CallJavascriptFunction(
        "BrowserOptions.setMetricsReportingSettingVisibility", visible);
  }
#endif
}

void BrowserOptionsHandler::SetupFontSizeSelector() {
  PrefService* pref_service = Profile::FromWebUI(web_ui())->GetPrefs();
  const PrefService::Preference* default_font_size =
      pref_service->FindPreference(prefs::kWebKitDefaultFontSize);
  const PrefService::Preference* default_fixed_font_size =
      pref_service->FindPreference(prefs::kWebKitDefaultFixedFontSize);

  base::DictionaryValue dict;
  dict.SetInteger("value",
                  pref_service->GetInteger(prefs::kWebKitDefaultFontSize));

  // The font size control displays the value of the default font size, but
  // setting it alters both the default font size and the default fixed font
  // size. So it must be disabled when either of those prefs is not user
  // modifiable.
  dict.SetBoolean("disabled",
      !default_font_size->IsUserModifiable() ||
      !default_fixed_font_size->IsUserModifiable());

  // This is a poor man's version of CoreOptionsHandler::CreateValueForPref,
  // adapted to consider two prefs. It may be better to refactor
  // CreateValueForPref so it can be called from here.
  if (default_font_size->IsManaged() || default_fixed_font_size->IsManaged()) {
      dict.SetString("controlledBy", "policy");
  } else if (default_font_size->IsExtensionControlled() ||
             default_fixed_font_size->IsExtensionControlled()) {
      dict.SetString("controlledBy", "extension");
  }

  web_ui()->CallJavascriptFunction("BrowserOptions.setFontSize", dict);
}

void BrowserOptionsHandler::SetupPageZoomSelector() {
  PrefService* pref_service = Profile::FromWebUI(web_ui())->GetPrefs();
  double default_zoom_level = pref_service->GetDouble(prefs::kDefaultZoomLevel);
  double default_zoom_factor =
      content::ZoomLevelToZoomFactor(default_zoom_level);

  // Generate a vector of zoom factors from an array of known presets along with
  // the default factor added if necessary.
  std::vector<double> zoom_factors =
      chrome_page_zoom::PresetZoomFactors(default_zoom_factor);

  // Iterate through the zoom factors and and build the contents of the
  // selector that will be sent to the javascript handler.
  // Each item in the list has the following parameters:
  // 1. Title (string).
  // 2. Value (double).
  // 3. Is selected? (bool).
  base::ListValue zoom_factors_value;
  for (std::vector<double>::const_iterator i = zoom_factors.begin();
       i != zoom_factors.end(); ++i) {
    base::ListValue* option = new base::ListValue();
    double factor = *i;
    int percent = static_cast<int>(factor * 100 + 0.5);
    option->Append(new base::StringValue(
        l10n_util::GetStringFUTF16Int(IDS_ZOOM_PERCENT, percent)));
    option->Append(new base::FundamentalValue(factor));
    bool selected = content::ZoomValuesEqual(factor, default_zoom_factor);
    option->Append(new base::FundamentalValue(selected));
    zoom_factors_value.Append(option);
  }

  web_ui()->CallJavascriptFunction(
      "BrowserOptions.setupPageZoomSelector", zoom_factors_value);
}

void BrowserOptionsHandler::SetupAutoOpenFileTypes() {
  // Set the hidden state for the AutoOpenFileTypesResetToDefault button.
  // We show the button if the user has any auto-open file types registered.
  DownloadManager* manager = BrowserContext::GetDownloadManager(
      web_ui()->GetWebContents()->GetBrowserContext());
  bool display = manager &&
      DownloadPrefs::FromDownloadManager(manager)->IsAutoOpenUsed();
  base::FundamentalValue value(display);
  web_ui()->CallJavascriptFunction(
      "BrowserOptions.setAutoOpenFileTypesDisplayed", value);
}

void BrowserOptionsHandler::SetupProxySettingsSection() {
#if !defined(OS_CHROMEOS)
  // Disable the button if proxy settings are managed by a sysadmin, overridden
  // by an extension, or the browser is running in Windows Ash (on Windows the
  // proxy settings dialog will open on the Windows desktop and be invisible
  // to a user in Ash).
  bool is_win_ash = false;
#if defined(OS_WIN)
  chrome::HostDesktopType desktop_type = helper::GetDesktopType(web_ui());
  is_win_ash = (desktop_type == chrome::HOST_DESKTOP_TYPE_ASH);
#endif
  PrefService* pref_service = Profile::FromWebUI(web_ui())->GetPrefs();
  const PrefService::Preference* proxy_config =
      pref_service->FindPreference(prefs::kProxy);
  bool is_extension_controlled = (proxy_config &&
                                  proxy_config->IsExtensionControlled());

  base::FundamentalValue disabled(is_win_ash || (proxy_config &&
                                  !proxy_config->IsUserModifiable()));
  base::FundamentalValue extension_controlled(is_extension_controlled);
  web_ui()->CallJavascriptFunction("BrowserOptions.setupProxySettingsSection",
                                   disabled, extension_controlled);

#endif  // !defined(OS_CHROMEOS)
}

void BrowserOptionsHandler::SetupManageCertificatesSection() {
#if defined(OS_WIN)
  // Disable the button if the settings page is displayed in Windows Ash,
  // otherwise the proxy settings dialog will open on the Windows desktop and
  // be invisible to a user in Ash.
  if (helper::GetDesktopType(web_ui()) == chrome::HOST_DESKTOP_TYPE_ASH) {
    base::FundamentalValue enabled(false);
    web_ui()->CallJavascriptFunction("BrowserOptions.enableCertificateButton",
                                     enabled);
  }
#endif  // defined(OS_WIN)
}

void BrowserOptionsHandler::SetupManagingSupervisedUsers() {
  bool has_users = !Profile::FromWebUI(web_ui())->
      GetPrefs()->GetDictionary(prefs::kManagedUsers)->empty();
  base::FundamentalValue has_users_value(has_users);
  web_ui()->CallJavascriptFunction(
      "BrowserOptions.updateManagesSupervisedUsers",
      has_users_value);
}

}  // namespace options
