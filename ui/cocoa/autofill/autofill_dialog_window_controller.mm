// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/autofill/autofill_dialog_window_controller.h"

#include "base/mac/foundation_util.h"
#include "base/mac/scoped_nsobject.h"
#include "base/strings/sys_string_conversions.h"
#include "chrome/browser/ui/autofill/autofill_dialog_view_delegate.h"
#include "chrome/browser/ui/cocoa/autofill/autofill_dialog_cocoa.h"
#include "chrome/browser/ui/cocoa/autofill/autofill_dialog_constants.h"
#import "chrome/browser/ui/cocoa/autofill/autofill_header.h"
#import "chrome/browser/ui/cocoa/autofill/autofill_input_field.h"
#import "chrome/browser/ui/cocoa/autofill/autofill_loading_shield_controller.h"
#import "chrome/browser/ui/cocoa/autofill/autofill_main_container.h"
#import "chrome/browser/ui/cocoa/autofill/autofill_overlay_controller.h"
#import "chrome/browser/ui/cocoa/autofill/autofill_section_container.h"
#import "chrome/browser/ui/cocoa/autofill/autofill_sign_in_container.h"
#import "chrome/browser/ui/cocoa/autofill/autofill_textfield.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_custom_window.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_view.h"
#include "grit/generated_resources.h"
#import "ui/base/cocoa/flipped_view.h"
#include "ui/base/cocoa/window_size_constants.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

const CGFloat kMinimumContentsHeight = 101;

}  // namespace

#pragma mark Field Editor

@interface AutofillDialogFieldEditor : NSTextView
@end


@implementation AutofillDialogFieldEditor

- (void)mouseDown:(NSEvent*)event {
  // Delegate _must_ be notified before mouseDown is complete, since it needs
  // to distinguish between mouseDown for already focused fields, and fields
  // that will receive focus as part of the mouseDown.
  AutofillTextField* textfield =
      base::mac::ObjCCastStrict<AutofillTextField>([self delegate]);
  [textfield onEditorMouseDown:self];
  [super mouseDown:event];
}

// Intercept key down messages and forward them to the text fields delegate.
// This needs to happen in the field editor, since it handles all keyDown
// messages for NSTextField.
- (void)keyDown:(NSEvent*)event {
  AutofillTextField* textfield =
      base::mac::ObjCCastStrict<AutofillTextField>([self delegate]);
  if ([[textfield inputDelegate] keyEvent:event
                                 forInput:textfield] != kKeyEventHandled) {
    [super keyDown:event];
  }
}

@end


#pragma mark Window Controller

@interface AutofillDialogWindowController ()

// Compute maximum allowed height for the dialog.
- (CGFloat)maxHeight;

// Update size constraints on sign-in container.
- (void)updateSignInSizeConstraints;

// Notification that the WebContent's view frame has changed.
- (void)onContentViewFrameDidChange:(NSNotification*)notification;

// Update whether or not the main container is hidden.
- (void)updateMainContainerVisibility;

@end


@implementation AutofillDialogWindowController (NSWindowDelegate)

- (id)windowWillReturnFieldEditor:(NSWindow*)window toObject:(id)client {
  AutofillTextField* textfield = base::mac::ObjCCast<AutofillTextField>(client);
  if (!textfield)
    return nil;

  if (!fieldEditor_) {
    fieldEditor_.reset([[AutofillDialogFieldEditor alloc] init]);
    [fieldEditor_ setFieldEditor:YES];
  }
  return fieldEditor_.get();
}

@end


@implementation AutofillDialogWindowController

- (id)initWithWebContents:(content::WebContents*)webContents
                   dialog:(autofill::AutofillDialogCocoa*)dialog {
  DCHECK(webContents);

  base::scoped_nsobject<ConstrainedWindowCustomWindow> window(
      [[ConstrainedWindowCustomWindow alloc]
          initWithContentRect:ui::kWindowSizeDeterminedLater]);

  if ((self = [super initWithWindow:window])) {
    [window setDelegate:self];
    webContents_ = webContents;
    dialog_ = dialog;

    header_.reset([[AutofillHeader alloc] initWithDelegate:dialog->delegate()]);

    mainContainer_.reset([[AutofillMainContainer alloc]
                             initWithDelegate:dialog->delegate()]);
    [mainContainer_ setTarget:self];

    signInContainer_.reset(
        [[AutofillSignInContainer alloc] initWithDialog:dialog]);
    [[signInContainer_ view] setHidden:YES];

    loadingShieldController_.reset(
        [[AutofillLoadingShieldController alloc] initWithDelegate:
            dialog->delegate()]);
    [[loadingShieldController_ view] setHidden:YES];

    overlayController_.reset(
        [[AutofillOverlayController alloc] initWithDelegate:
            dialog->delegate()]);
    [[overlayController_ view] setHidden:YES];

    // This needs a flipped content view because otherwise the size
    // animation looks odd. However, replacing the contentView for constrained
    // windows does not work - it does custom rendering.
    base::scoped_nsobject<NSView> flippedContentView(
        [[FlippedView alloc] initWithFrame:
            [[[self window] contentView] frame]]);
    [flippedContentView setSubviews:
        @[[header_ view],
          [mainContainer_ view],
          [signInContainer_ view],
          [loadingShieldController_ view],
          [overlayController_ view]]];
    [flippedContentView setAutoresizingMask:
        (NSViewWidthSizable | NSViewHeightSizable)];
    [[[self window] contentView] addSubview:flippedContentView];
    [mainContainer_ setAnchorView:[header_ anchorView]];
  }
  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [super dealloc];
}

- (CGFloat)maxHeight {
  NSRect dialogFrameRect = [[self window] frame];
  NSRect browserFrameRect =
      [webContents_->GetView()->GetTopLevelNativeWindow() frame];
  dialogFrameRect.size.height =
      NSMaxY(dialogFrameRect) - NSMinY(browserFrameRect);
  dialogFrameRect = [[self window] contentRectForFrameRect:dialogFrameRect];
  return NSHeight(dialogFrameRect);
}

- (void)updateSignInSizeConstraints {
  // Adjust for the size of the header.
  CGFloat headerHeight = [[header_ view] frame].size.height;
  CGFloat minHeight = kMinimumContentsHeight - headerHeight;
  CGFloat maxHeight = std::max([self maxHeight] - headerHeight, minHeight);
  CGFloat width = NSWidth([[[self window] contentView] frame]);

  [signInContainer_ constrainSizeToMinimum:NSMakeSize(width, minHeight)
                                   maximum:NSMakeSize(width, maxHeight)];
}

- (void)onContentViewFrameDidChange:(NSNotification*)notification {
  [self updateSignInSizeConstraints];
  if ([[signInContainer_ view] isHidden])
    [self requestRelayout];
}

- (void)updateMainContainerVisibility {
  BOOL visible =
      [[loadingShieldController_ view] isHidden] &&
      [[overlayController_ view] isHidden] &&
      [[signInContainer_ view] isHidden];
  [[mainContainer_ view] setHidden:!visible];
}

- (void)cancelRelayout {
  [NSObject cancelPreviousPerformRequestsWithTarget:self
                                           selector:@selector(performLayout)
                                             object:nil];
}

- (void)requestRelayout {
  [self cancelRelayout];
  [self performSelector:@selector(performLayout) withObject:nil afterDelay:0.0];
}

- (NSSize)preferredSize {
  NSSize size;

  if (![[overlayController_ view] isHidden]) {
    size.height = [overlayController_ heightForWidth:size.width];
  } else {
    // Overall size is determined by either main container or sign in view.
    if ([[signInContainer_ view] isHidden])
      size = [mainContainer_ preferredSize];
    else
      size = [signInContainer_ preferredSize];

    // Always make room for the header.
    size.height += [header_ heightForWidth:size.width];
  }

  // Show as much of the main view as is possible without going past the
  // bottom of the browser window.
  size.height = std::min(size.height, [self maxHeight]);

  return size;
}

- (void)performLayout {
  NSRect contentRect = NSZeroRect;
  contentRect.size = [self preferredSize];

  CGFloat headerHeight = [header_ heightForWidth:NSWidth(contentRect)];
  NSRect headerRect, mainRect;
  NSDivideRect(contentRect, &headerRect, &mainRect, headerHeight, NSMinYEdge);

  [[header_ view] setFrame:headerRect];
  [header_ performLayout];

  if ([[signInContainer_ view] isHidden]) {
    [[mainContainer_ view] setFrame:mainRect];
    [mainContainer_ performLayout];
  } else {
    [[signInContainer_ view] setFrame:mainRect];
  }

  [[loadingShieldController_ view] setFrame:contentRect];
  [loadingShieldController_ performLayout];

  [[overlayController_ view] setFrame:contentRect];
  [overlayController_ performLayout];

  NSRect frameRect = [[self window] frameRectForContentRect:contentRect];
  [[self window] setFrame:frameRect display:YES];
  [[self window] recalculateKeyViewLoop];
}

- (IBAction)accept:(id)sender {
  if ([mainContainer_ validate])
    dialog_->delegate()->OnAccept();
  else
    [mainContainer_ makeFirstInvalidInputFirstResponder];
}

- (IBAction)cancel:(id)sender {
  dialog_->delegate()->OnCancel();
  dialog_->PerformClose();
}

- (void)show {
  // Resizing the browser causes the ConstrainedWindow to move.
  // Observe that to allow resizes based on browser size.
  // NOTE: This MUST come last after all initial setup is done, because there
  // is an immediate notification post registration.
  DCHECK([self window]);
  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(onContentViewFrameDidChange:)
             name:NSWindowDidMoveNotification
           object:[self window]];

  [self updateAccountChooser];
  [self updateNotificationArea];
  [self requestRelayout];
}

- (void)hide {
  dialog_->delegate()->OnCancel();
  dialog_->PerformClose();
}

- (void)updateNotificationArea {
  [mainContainer_ updateNotificationArea];
}

- (void)updateAccountChooser {
  [header_ update];
  [mainContainer_ updateLegalDocuments];
  [loadingShieldController_ update];
  [self updateMainContainerVisibility];
}

- (void)updateButtonStrip {
  // For the duration of the overlay, hide the main contents and the header.
  // This prevents the currently focused text field "shining through". No need
  // to remember previous state, because the overlay view is always the last
  // state of the dialog.
  [overlayController_ updateState];
  [[header_ view] setHidden:![[overlayController_ view] isHidden]];
  [self updateMainContainerVisibility];
}

- (void)updateSection:(autofill::DialogSection)section {
  [[mainContainer_ sectionForId:section] update];
  [mainContainer_ updateSaveInChrome];
}

- (void)fillSection:(autofill::DialogSection)section
           forInput:(const autofill::DetailInput&)input {
  [[mainContainer_ sectionForId:section] fillForInput:input];
  [mainContainer_ updateSaveInChrome];
}

- (content::NavigationController*)showSignIn {
  [self updateSignInSizeConstraints];
  [signInContainer_ loadSignInPage];
  [[signInContainer_ view] setHidden:NO];
  [self updateMainContainerVisibility];
  [self requestRelayout];

  return [signInContainer_ navigationController];
}

- (void)getInputs:(autofill::DetailOutputMap*)output
       forSection:(autofill::DialogSection)section {
  [[mainContainer_ sectionForId:section] getInputs:output];
}

- (NSString*)getCvc {
  autofill::DialogSection section = autofill::SECTION_CC;
  NSString* value = [[mainContainer_ sectionForId:section] suggestionText];
  if (!value) {
    section = autofill::SECTION_CC_BILLING;
    value = [[mainContainer_ sectionForId:section] suggestionText];
  }
  return value;
}

- (BOOL)saveDetailsLocally {
  return [mainContainer_ saveDetailsLocally];
}

- (void)hideSignIn {
  [[signInContainer_ view] setHidden:YES];
  [self updateMainContainerVisibility];
  [self requestRelayout];
}

- (void)modelChanged {
  [mainContainer_ modelChanged];
}

- (void)updateErrorBubble {
  [mainContainer_ updateErrorBubble];
}

- (void)onSignInResize:(NSSize)size {
  [signInContainer_ setPreferredSize:size];
  [self requestRelayout];
}

@end


@implementation AutofillDialogWindowController (TestableAutofillDialogView)

- (void)setTextContents:(NSString*)text
               forInput:(const autofill::DetailInput&)input {
  for (size_t i = autofill::SECTION_MIN; i <= autofill::SECTION_MAX; ++i) {
    autofill::DialogSection section = static_cast<autofill::DialogSection>(i);
    // TODO(groby): Need to find the section for an input directly - wasteful.
    [[mainContainer_ sectionForId:section] setFieldValue:text forInput:input];
  }
}

- (void)setTextContents:(NSString*)text
 ofSuggestionForSection:(autofill::DialogSection)section {
  [[mainContainer_ sectionForId:section] setSuggestionFieldValue:text];
}

- (void)activateFieldForInput:(const autofill::DetailInput&)input {
  for (size_t i = autofill::SECTION_MIN; i <= autofill::SECTION_MAX; ++i) {
    autofill::DialogSection section = static_cast<autofill::DialogSection>(i);
    [[mainContainer_ sectionForId:section] activateFieldForInput:input];
  }
}

- (content::WebContents*)getSignInWebContents {
  return [signInContainer_ webContents];
}

- (BOOL)isShowingOverlay {
  return ![[overlayController_ view] isHidden];
}

@end
