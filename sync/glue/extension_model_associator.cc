// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/glue/extension_model_associator.h"

#include "base/logging.h"
#include "chrome/browser/chrome_thread.h"
#include "chrome/browser/sync/glue/extension_data.h"
#include "chrome/browser/sync/glue/extension_sync_traits.h"
#include "chrome/browser/sync/glue/extension_sync.h"
#include "chrome/browser/sync/protocol/extension_specifics.pb.h"

namespace browser_sync {

ExtensionModelAssociator::ExtensionModelAssociator(
    ProfileSyncService* sync_service) : sync_service_(sync_service) {
  DCHECK(ChromeThread::CurrentlyOn(ChromeThread::UI));
  DCHECK(sync_service_);
}

ExtensionModelAssociator::~ExtensionModelAssociator() {
  DCHECK(ChromeThread::CurrentlyOn(ChromeThread::UI));
}

bool ExtensionModelAssociator::AssociateModels() {
  DCHECK(ChromeThread::CurrentlyOn(ChromeThread::UI));
  const ExtensionSyncTraits traits = GetExtensionSyncTraits();

  ExtensionDataMap extension_data_map;
  if (!SlurpExtensionData(traits, sync_service_, &extension_data_map)) {
    return false;
  }
  if (!FlushExtensionData(traits, extension_data_map, sync_service_)) {
    return false;
  }

  return true;
}

bool ExtensionModelAssociator::DisassociateModels() {
  DCHECK(ChromeThread::CurrentlyOn(ChromeThread::UI));
  // Nothing to do.
  return true;
}

bool ExtensionModelAssociator::SyncModelHasUserCreatedNodes(bool* has_nodes) {
  DCHECK(ChromeThread::CurrentlyOn(ChromeThread::UI));
  const ExtensionSyncTraits traits = GetExtensionSyncTraits();
  return RootNodeHasChildren(traits.root_node_tag, sync_service_, has_nodes);
}

}  // namespace browser_sync
