// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PRERENDER_PRERENDER_FINAL_STATUS_H_
#define CHROME_BROWSER_PRERENDER_PRERENDER_FINAL_STATUS_H_

namespace prerender {

// FinalStatus indicates whether |this| was used, or why it was cancelled.
// NOTE: New values need to be appended, since they are used in histograms.
enum FinalStatus {
  FINAL_STATUS_USED = 0,
  FINAL_STATUS_TIMED_OUT = 1,
  FINAL_STATUS_EVICTED = 2,
  FINAL_STATUS_MANAGER_SHUTDOWN = 3,
  // Obsolete: FINAL_STATUS_CLOSED = 4
  FINAL_STATUS_CREATE_NEW_WINDOW = 5,
  FINAL_STATUS_PROFILE_DESTROYED = 6,
  FINAL_STATUS_APP_TERMINATING = 7,
  FINAL_STATUS_JAVASCRIPT_ALERT = 8,
  FINAL_STATUS_AUTH_NEEDED = 9,
  FINAL_STATUS_HTTPS = 10,
  FINAL_STATUS_DOWNLOAD = 11,
  FINAL_STATUS_MEMORY_LIMIT_EXCEEDED = 12,
  FINAL_STATUS_JS_OUT_OF_MEMORY = 13,
  // Obsolete: FINAL_STATUS_RENDERER_UNRESPONSIVE = 14,
  FINAL_STATUS_TOO_MANY_PROCESSES = 15,
  FINAL_STATUS_RATE_LIMIT_EXCEEDED = 16,
  // Obsolete: FINAL_STATUS_PENDING_SKIPPED = 17,
  FINAL_STATUS_CONTROL_GROUP = 18,
  FINAL_STATUS_HTML5_MEDIA = 19,
  FINAL_STATUS_SOURCE_RENDER_VIEW_CLOSED = 20,
  FINAL_STATUS_RENDERER_CRASHED = 21,
  FINAL_STATUS_UNSUPPORTED_SCHEME = 22,
  FINAL_STATUS_INVALID_HTTP_METHOD = 23,
  FINAL_STATUS_WINDOW_PRINT = 24,
  FINAL_STATUS_RECENTLY_VISITED = 25,
  FINAL_STATUS_WINDOW_OPENER = 26,
  FINAL_STATUS_PAGE_ID_CONFLICT = 27,
  FINAL_STATUS_SAFE_BROWSING = 28,
  FINAL_STATUS_FRAGMENT_MISMATCH = 29,
  FINAL_STATUS_SSL_CLIENT_CERTIFICATE_REQUESTED = 30,
  FINAL_STATUS_MAX,
};

void RecordFinalStatus(FinalStatus final_status);

// Return a human-readable name for |final_status|. |final_status|
// is expected to be a valid value.
const char* const NameFromFinalStatus(FinalStatus final_status);

}  // namespace prerender

#endif  // CHROME_BROWSER_PRERENDER_PRERENDER_FINAL_STATUS_H_
