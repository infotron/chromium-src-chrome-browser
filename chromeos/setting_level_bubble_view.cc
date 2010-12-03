// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/setting_level_bubble_view.h"

#include <string>

#include "base/logging.h"
#include "gfx/canvas.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "views/controls/progress_bar.h"

using views::Background;
using views::View;
using views::Widget;

namespace {

// Bubble metrics.
const int kWidth = 300, kHeight = 75;
const int kMargin = 25;
const int kProgressBarHeight = 20;

}  // namespace

namespace chromeos {

SettingLevelBubbleView::SettingLevelBubbleView()
    : progress_bar_(NULL),
      icon_(NULL) {
}

void SettingLevelBubbleView::Init(SkBitmap* icon, int level_percent) {
  DCHECK(level_percent >= 0 && level_percent <= 100);
  icon_ = icon;
  progress_bar_ = new views::ProgressBar();
  AddChildView(progress_bar_);
  Update(level_percent);
}

void SettingLevelBubbleView::Update(int level_percent) {
  DCHECK(level_percent >= 0 && level_percent <= 100);
  progress_bar_->SetProgress(level_percent);
}

void SettingLevelBubbleView::Paint(gfx::Canvas* canvas) {
  views::View::Paint(canvas);
  canvas->DrawBitmapInt(*icon_,
                        icon_->width(), (height() - icon_->height()) / 2);
}

void SettingLevelBubbleView::Layout() {
  progress_bar_->SetBounds(icon_->width() + kMargin * 2,
                           (height() - kProgressBarHeight) / 2,
                           width() - icon_->width() - kMargin * 3,
                           kProgressBarHeight);
}

gfx::Size SettingLevelBubbleView::GetPreferredSize() {
  return gfx::Size(kWidth, kHeight);
}

}  // namespace chromeos
