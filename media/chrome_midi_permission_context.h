// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_CHROME_MIDI_PERMISSION_CONTEXT_H_
#define CHROME_BROWSER_MEDIA_CHROME_MIDI_PERMISSION_CONTEXT_H_

#include "base/containers/scoped_ptr_hash_map.h"
#include "base/memory/scoped_ptr.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"

namespace content {
class WebContents;
}

class GURL;
class MidiPermissionRequest;
class PermissionQueueController;
class PermissionRequestID;
class Profile;

// This class manages MIDI permissions flow. Used on the UI thread.
class ChromeMidiPermissionContext : public KeyedService {
 public:
  explicit ChromeMidiPermissionContext(Profile* profile);
  virtual ~ChromeMidiPermissionContext();

  // KeyedService methods:
  virtual void Shutdown() OVERRIDE;

  // Request to ask users permission about MIDI.
  void RequestMidiSysExPermission(
      int render_process_id,
      int render_view_id,
      int bridge_id,
      const GURL& requesting_frame,
      bool user_gesture,
      const content::BrowserContext::MidiSysExPermissionCallback& callback);

  // Cancel a pending MIDI permission request.
  void CancelMidiSysExPermissionRequest(int render_process_id,
                                        int render_view_id,
                                        int bridge_id,
                                        const GURL& requesting_frame);

  // Called when the permission decision is made. If a permissions prompt is
  // shown to the user it will be called when the user selects an option
  // from that prompt.
  void NotifyPermissionSet(
      const PermissionRequestID& id,
      const GURL& requesting_frame,
      const content::BrowserContext::MidiSysExPermissionCallback& callback,
      bool allowed);

 private:
  friend class MidiPermissionRequest;

  // Decide whether the permission should be granted.
  // Calls PermissionDecided if permission can be decided non-interactively,
  // or NotifyPermissionSet if permission decided by presenting an infobar.
  void DecidePermission(
      content::WebContents* web_contents,
      const PermissionRequestID& id,
      const GURL& requesting_frame,
      const GURL& embedder,
      bool user_gesture,
      const content::BrowserContext::MidiSysExPermissionCallback& callback);

  // Called when permission is granted without interactively asking the user.
  void PermissionDecided(
      const PermissionRequestID& id,
      const GURL& requesting_frame,
      const GURL& embedder,
      const content::BrowserContext::MidiSysExPermissionCallback& callback,
      bool allowed);

  // Return an instance of the infobar queue controller, creating it if needed.
  PermissionQueueController* GetQueueController();

  // Removes any pending InfoBar request.
  void CancelPendingInfobarRequest(const PermissionRequestID& id);

  // Notify the context that a particular request object is no longer needed.
  void RequestFinished(MidiPermissionRequest* request);

  Profile* const profile_;
  bool shutting_down_;
  scoped_ptr<PermissionQueueController> permission_queue_controller_;

  base::ScopedPtrHashMap<std::string, MidiPermissionRequest> pending_requests_;

  DISALLOW_COPY_AND_ASSIGN(ChromeMidiPermissionContext);
};

#endif  // CHROME_BROWSER_MEDIA_CHROME_MIDI_PERMISSION_CONTEXT_H_
