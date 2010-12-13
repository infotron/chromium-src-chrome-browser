// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SIDEBAR_SIDEBAR_CONTAINER_H_
#define CHROME_BROWSER_SIDEBAR_SIDEBAR_CONTAINER_H_

#include <string>

#include "base/basictypes.h"
#include "base/scoped_ptr.h"
#include "base/string16.h"
#include "chrome/browser/tab_contents/tab_contents_delegate.h"

class BrowserWindow;
class Profile;
class RenderViewHost;
class SkBitmap;
class TabContents;

///////////////////////////////////////////////////////////////////////////////
// SidebarContainer
//
//  Stores one particular sidebar state: sidebar's content, its content id,
//  tab it is linked to, mini tab icon, title etc.
//
class SidebarContainer
    : public TabContentsDelegate {
 public:
  // Interface to implement to listen for sidebar update notification.
  class Delegate {
   public:
    Delegate() {}
    virtual ~Delegate() {}
    virtual void UpdateSidebar(SidebarContainer* host) = 0;
   private:
    DISALLOW_COPY_AND_ASSIGN(Delegate);
  };

  SidebarContainer(TabContents* tab, const std::string& content_id,
                   Delegate* delegate);
  virtual ~SidebarContainer();

  // Called right before destroying this sidebar.
  // Does all the necessary cleanup.
  void SidebarClosing();

  // Returns sidebar's content id.
  const std::string& content_id() const { return content_id_; }

  // Returns TabContents sidebar is linked to.
  TabContents* tab_contents() const { return tab_; }

  // Returns sidebar's TabContents.
  TabContents* sidebar_contents() const { return sidebar_contents_.get(); }

  // Accessor for the badge text.
  const string16& badge_text() const { return badge_text_; }

  // Accessor for the icon.
  const SkBitmap& icon() const { return *icon_; }

  // Accessor for the title.
  const string16& title() const { return title_; }

  // Functions supporting chrome.experimental.sidebar API.

  // Notifies hosting window that this sidebar was expanded.
  void Show();

  // Notifies hosting window that this sidebar was expanded.
  void Expand();

  // Notifies hosting window that this sidebar was collapsed.
  void Collapse();

  // Navigates sidebar contents to the |url|.
  void Navigate(const GURL& url);

  // Changes sidebar's badge text.
  void SetBadgeText(const string16& badge_text);

  // Changes sidebar's icon.
  void SetIcon(const SkBitmap& bitmap);

  // Changes sidebar's title.
  void SetTitle(const string16& title);

 private:
  // Overridden from TabContentsDelegate.
  virtual void OpenURLFromTab(TabContents* source,
                              const GURL& url,
                              const GURL& referrer,
                              WindowOpenDisposition disposition,
                              PageTransition::Type transition) {}
  virtual void NavigationStateChanged(const TabContents* source,
                                      unsigned changed_flags) {}
  virtual void AddNewContents(TabContents* source,
                              TabContents* new_contents,
                              WindowOpenDisposition disposition,
                              const gfx::Rect& initial_pos,
                              bool user_gesture) {}
  virtual void ActivateContents(TabContents* contents) {}
  virtual void DeactivateContents(TabContents* contents) {}
  virtual void LoadingStateChanged(TabContents* source) {}
  virtual void CloseContents(TabContents* source) {}
  virtual void MoveContents(TabContents* source, const gfx::Rect& pos) {}
  virtual bool IsPopup(const TabContents* source) const;
  virtual void URLStarredChanged(TabContents* source, bool starred) {}
  virtual void UpdateTargetURL(TabContents* source, const GURL& url) {}
  virtual void ToolbarSizeChanged(TabContents* source, bool is_animating) {}

  // Contents of the tab this sidebar is linked to.
  TabContents* tab_;

  // Sidebar's content id. There might be more than one sidebar liked to each
  // particular tab and they are identified by their unique content id.
  const std::string content_id_;

  // Sidebar update notification listener.
  Delegate* delegate_;

  // Sidebar contents.
  scoped_ptr<TabContents> sidebar_contents_;

  // Badge text displayed on the sidebar's mini tab.
  string16 badge_text_;

  // Icon displayed on the sidebar's mini tab.
  scoped_ptr<SkBitmap> icon_;

  // Sidebar's title, displayed as a tooltip for sidebar's mini tab.
  string16 title_;

  DISALLOW_COPY_AND_ASSIGN(SidebarContainer);
};

#endif  // CHROME_BROWSER_SIDEBAR_SIDEBAR_CONTAINER_H_

