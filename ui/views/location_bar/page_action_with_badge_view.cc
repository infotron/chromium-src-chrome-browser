// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/location_bar/page_action_with_badge_view.h"

#include "chrome/browser/extensions/extension_action.h"
#include "chrome/browser/ui/views/location_bar/page_action_image_view.h"
#include "ui/accessibility/ax_view_state.h"

using content::WebContents;

PageActionWithBadgeView::PageActionWithBadgeView(
    PageActionImageView* image_view) {
  image_view_ = image_view;
  AddChildView(image_view_);
}

void PageActionWithBadgeView::GetAccessibleState(
    ui::AXViewState* state) {
  state->role = ui::AX_ROLE_GROUP;
}

gfx::Size PageActionWithBadgeView::GetPreferredSize() const {
  return gfx::Size(ExtensionAction::kPageActionIconMaxSize,
                   ExtensionAction::kPageActionIconMaxSize);
}

void PageActionWithBadgeView::Layout() {
  // We have 25 pixels of vertical space in the Omnibox to play with, so even
  // sized icons (such as 16x16) have either a 5 or a 4 pixel whitespace
  // (padding) above and below. It looks better to have the extra pixel above
  // the icon than below it, so we add a pixel. http://crbug.com/25708.
  const gfx::ImageSkia& image = image_view()->GetImage();
  int y = (image.height() + 1) % 2;  // Even numbers: 1px padding. Odd: 0px.
  image_view_->SetBounds(0, y, width(), height());
}

void PageActionWithBadgeView::UpdateVisibility(WebContents* contents,
                                               const GURL& url) {
  image_view_->UpdateVisibility(contents, url);
  SetVisible(image_view_->visible());
}
