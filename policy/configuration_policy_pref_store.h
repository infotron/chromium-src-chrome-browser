// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_POLICY_CONFIGURATION_POLICY_PREF_STORE_H_
#define CHROME_BROWSER_POLICY_CONFIGURATION_POLICY_PREF_STORE_H_
#pragma once

#include <set>
#include <string>

#include "base/basictypes.h"
#include "base/observer_list.h"
#include "base/scoped_ptr.h"
#include "base/values.h"
#include "chrome/browser/policy/configuration_policy_provider.h"
#include "chrome/browser/policy/configuration_policy_store_interface.h"
#include "chrome/common/notification_observer.h"
#include "chrome/common/notification_registrar.h"
#include "chrome/common/pref_store.h"

class Profile;

namespace policy {

class ConfigurationPolicyPrefKeeper;

// An implementation of PrefStore that bridges policy settings as read from a
// ConfigurationPolicyProvider to preferences.
class ConfigurationPolicyPrefStore : public PrefStore,
                                     public NotificationObserver {
 public:
  // The ConfigurationPolicyPrefStore does not take ownership of the
  // passed-in |provider|.
  explicit ConfigurationPolicyPrefStore(ConfigurationPolicyProvider* provider);
  virtual ~ConfigurationPolicyPrefStore();

  // PrefStore methods:
  virtual void AddObserver(Observer* observer);
  virtual void RemoveObserver(Observer* observer);
  virtual bool IsInitializationComplete() const;
  virtual ReadResult GetValue(const std::string& key, Value** result) const;

  // Creates a ConfigurationPolicyPrefStore that reads managed platform policy.
  static ConfigurationPolicyPrefStore* CreateManagedPlatformPolicyPrefStore();

  // Creates a ConfigurationPolicyPrefStore that supplies policy from
  // the device management server.
  static ConfigurationPolicyPrefStore* CreateDeviceManagementPolicyPrefStore(
      Profile* profile);

  // Creates a ConfigurationPolicyPrefStore that reads recommended policy.
  static ConfigurationPolicyPrefStore* CreateRecommendedPolicyPrefStore();

  // Returns the default policy definition list for Chrome.
  static const ConfigurationPolicyProvider::PolicyDefinitionList*
      GetChromePolicyDefinitionList();

 private:
  // TODO(mnissler): Remove after provider has proper observer interface.
  // NotificationObserver overrides:
  void Observe(NotificationType type,
               const NotificationSource& source,
               const NotificationDetails& details);

  // Refreshes policy information, rereading policy from the provider and
  // sending out change notifications as appropriate.
  void Refresh();

  static const ConfigurationPolicyProvider::PolicyDefinitionList
      kPolicyDefinitionList;

  // The policy provider from which policy settings are read.
  ConfigurationPolicyProvider* provider_;

  // Initialization status as reported by the policy provider the last time we
  // queried it.
  bool initialization_complete_;

  // Current policy preferences.
  scoped_ptr<ConfigurationPolicyPrefKeeper> policy_keeper_;

  // TODO(mnissler): Remove after provider has proper observer interface.
  NotificationRegistrar registrar_;

  ObserverList<Observer, true> observers_;

  DISALLOW_COPY_AND_ASSIGN(ConfigurationPolicyPrefStore);
};

}  // namespace policy

#endif  // CHROME_BROWSER_POLICY_CONFIGURATION_POLICY_PREF_STORE_H_
