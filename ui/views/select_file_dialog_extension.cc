// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/select_file_dialog_extension.h"

#include "apps/shell_window.h"
#include "apps/shell_window_registry.h"
#include "apps/ui/native_app_window.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/memory/singleton.h"
#include "base/message_loop/message_loop.h"
#include "chrome/browser/chromeos/file_manager/app_id.h"
#include "chrome/browser/chromeos/file_manager/fileapi_util.h"
#include "chrome/browser/chromeos/file_manager/select_file_dialog_util.h"
#include "chrome/browser/chromeos/file_manager/url_util.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_system.h"
#include "chrome/browser/extensions/extension_view_host.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/chrome_select_file_policy.h"
#include "chrome/browser/ui/host_desktop.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/extensions/extension_dialog.h"
#include "chrome/common/pref_names.h"
#include "content/public/browser/browser_thread.h"
#include "ui/base/base_window.h"
#include "ui/shell_dialogs/selected_file_info.h"
#include "ui/views/widget/widget.h"

using apps::ShellWindow;
using content::BrowserThread;

namespace {

const int kFileManagerWidth = 972;  // pixels
const int kFileManagerHeight = 640;  // pixels
const int kFileManagerMinimumWidth = 320;  // pixels
const int kFileManagerMinimumHeight = 240;  // pixels

// Holds references to file manager dialogs that have callbacks pending
// to their listeners.
class PendingDialog {
 public:
  static PendingDialog* GetInstance();
  void Add(SelectFileDialogExtension::RoutingID id,
           scoped_refptr<SelectFileDialogExtension> dialog);
  void Remove(SelectFileDialogExtension::RoutingID id);
  scoped_refptr<SelectFileDialogExtension> Find(
      SelectFileDialogExtension::RoutingID id);

 private:
  friend struct DefaultSingletonTraits<PendingDialog>;
  typedef std::map<SelectFileDialogExtension::RoutingID,
                   scoped_refptr<SelectFileDialogExtension> > Map;
  Map map_;
};

// static
PendingDialog* PendingDialog::GetInstance() {
  return Singleton<PendingDialog>::get();
}

void PendingDialog::Add(SelectFileDialogExtension::RoutingID id,
                        scoped_refptr<SelectFileDialogExtension> dialog) {
  DCHECK(dialog.get());
  if (map_.find(id) == map_.end())
    map_.insert(std::make_pair(id, dialog));
  else
    DLOG(WARNING) << "Duplicate pending dialog " << id;
}

void PendingDialog::Remove(SelectFileDialogExtension::RoutingID id) {
  map_.erase(id);
}

scoped_refptr<SelectFileDialogExtension> PendingDialog::Find(
    SelectFileDialogExtension::RoutingID id) {
  Map::const_iterator it = map_.find(id);
  if (it == map_.end())
    return NULL;
  return it->second;
}

}  // namespace

/////////////////////////////////////////////////////////////////////////////

// static
SelectFileDialogExtension::RoutingID
SelectFileDialogExtension::GetRoutingIDFromWebContents(
    const content::WebContents* web_contents) {
  // Use the raw pointer value as the identifier. Previously we have used the
  // tab ID for the purpose, but some web_contents, especially those of the
  // packaged apps, don't have tab IDs assigned.
  return web_contents;
}

// TODO(jamescook): Move this into a new file shell_dialogs_chromeos.cc
// static
SelectFileDialogExtension* SelectFileDialogExtension::Create(
    Listener* listener,
    ui::SelectFilePolicy* policy) {
  return new SelectFileDialogExtension(listener, policy);
}

SelectFileDialogExtension::SelectFileDialogExtension(
    Listener* listener,
    ui::SelectFilePolicy* policy)
    : SelectFileDialog(listener, policy),
      has_multiple_file_type_choices_(false),
      routing_id_(),
      profile_(NULL),
      owner_window_(NULL),
      selection_type_(CANCEL),
      selection_index_(0),
      params_(NULL) {
}

SelectFileDialogExtension::~SelectFileDialogExtension() {
  if (extension_dialog_.get())
    extension_dialog_->ObserverDestroyed();
}

bool SelectFileDialogExtension::IsRunning(
    gfx::NativeWindow owner_window) const {
  return owner_window_ == owner_window;
}

void SelectFileDialogExtension::ListenerDestroyed() {
  listener_ = NULL;
  params_ = NULL;
  PendingDialog::GetInstance()->Remove(routing_id_);
}

void SelectFileDialogExtension::ExtensionDialogClosing(
    ExtensionDialog* /*dialog*/) {
  profile_ = NULL;
  owner_window_ = NULL;
  // Release our reference to the underlying dialog to allow it to close.
  extension_dialog_ = NULL;
  PendingDialog::GetInstance()->Remove(routing_id_);
  // Actually invoke the appropriate callback on our listener.
  NotifyListener();
}

void SelectFileDialogExtension::ExtensionTerminated(
    ExtensionDialog* dialog) {
  // The extension would have been unloaded because of the termination,
  // reload it.
  std::string extension_id = dialog->host()->extension()->id();
  // Reload the extension after a bit; the extension may not have been unloaded
  // yet. We don't want to try to reload the extension only to have the Unload
  // code execute after us and re-unload the extension.
  //
  // TODO(rkc): This is ugly. The ideal solution is that we shouldn't need to
  // reload the extension at all - when we try to open the extension the next
  // time, the extension subsystem would automatically reload it for us. At
  // this time though this is broken because of some faulty wiring in
  // extensions::ProcessManager::CreateViewHost. Once that is fixed, remove
  // this.
  if (profile_) {
    base::MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(&ExtensionService::ReloadExtension,
                   base::Unretained(extensions::ExtensionSystem::Get(profile_)
                                        ->extension_service()),
                   extension_id));
  }

  dialog->GetWidget()->Close();
}

// static
void SelectFileDialogExtension::OnFileSelected(
    RoutingID routing_id,
    const ui::SelectedFileInfo& file,
    int index) {
  scoped_refptr<SelectFileDialogExtension> dialog =
      PendingDialog::GetInstance()->Find(routing_id);
  if (!dialog.get())
    return;
  dialog->selection_type_ = SINGLE_FILE;
  dialog->selection_files_.clear();
  dialog->selection_files_.push_back(file);
  dialog->selection_index_ = index;
}

// static
void SelectFileDialogExtension::OnMultiFilesSelected(
    RoutingID routing_id,
    const std::vector<ui::SelectedFileInfo>& files) {
  scoped_refptr<SelectFileDialogExtension> dialog =
      PendingDialog::GetInstance()->Find(routing_id);
  if (!dialog.get())
    return;
  dialog->selection_type_ = MULTIPLE_FILES;
  dialog->selection_files_ = files;
  dialog->selection_index_ = 0;
}

// static
void SelectFileDialogExtension::OnFileSelectionCanceled(RoutingID routing_id) {
  scoped_refptr<SelectFileDialogExtension> dialog =
      PendingDialog::GetInstance()->Find(routing_id);
  if (!dialog.get())
    return;
  dialog->selection_type_ = CANCEL;
  dialog->selection_files_.clear();
  dialog->selection_index_ = 0;
}

content::RenderViewHost* SelectFileDialogExtension::GetRenderViewHost() {
  if (extension_dialog_.get())
    return extension_dialog_->host()->render_view_host();
  return NULL;
}

void SelectFileDialogExtension::NotifyListener() {
  if (!listener_)
    return;
  switch (selection_type_) {
    case CANCEL:
      listener_->FileSelectionCanceled(params_);
      break;
    case SINGLE_FILE:
      listener_->FileSelectedWithExtraInfo(selection_files_[0],
                                           selection_index_,
                                           params_);
      break;
    case MULTIPLE_FILES:
      listener_->MultiFilesSelectedWithExtraInfo(selection_files_, params_);
      break;
    default:
      NOTREACHED();
      break;
  }
}

void SelectFileDialogExtension::AddPending(RoutingID routing_id) {
  PendingDialog::GetInstance()->Add(routing_id, this);
}

// static
bool SelectFileDialogExtension::PendingExists(RoutingID routing_id) {
  return PendingDialog::GetInstance()->Find(routing_id).get() != NULL;
}

bool SelectFileDialogExtension::HasMultipleFileTypeChoicesImpl() {
  return has_multiple_file_type_choices_;
}

void SelectFileDialogExtension::SelectFileImpl(
    Type type,
    const base::string16& title,
    const base::FilePath& default_path,
    const FileTypeInfo* file_types,
    int file_type_index,
    const base::FilePath::StringType& default_extension,
    gfx::NativeWindow owner_window,
    void* params) {
  if (owner_window_) {
    LOG(ERROR) << "File dialog already in use!";
    return;
  }

  // The base window to associate the dialog with.
  ui::BaseWindow* base_window = NULL;

  // The web contents to associate the dialog with.
  content::WebContents* web_contents = NULL;

  // To get the base_window and profile, either a Browser or ShellWindow is
  // needed.
  Browser* owner_browser =  NULL;
  ShellWindow* shell_window = NULL;

  // If owner_window is supplied, use that to find a browser or a shell window.
  if (owner_window) {
    owner_browser = chrome::FindBrowserWithWindow(owner_window);
    if (!owner_browser) {
      // If an owner_window was supplied but we couldn't find a browser, this
      // could be for a shell window.
      shell_window = apps::ShellWindowRegistry::
          GetShellWindowForNativeWindowAnyProfile(owner_window);
    }
  }

  if (shell_window) {
    DCHECK(!shell_window->window_type_is_panel());
    base_window = shell_window->GetBaseWindow();
    web_contents = shell_window->web_contents();
  } else {
    // If the owning window is still unknown, this could be a background page or
    // and extension popup. Use the last active browser.
    if (!owner_browser) {
      owner_browser =
          chrome::FindLastActiveWithHostDesktopType(chrome::GetActiveDesktop());
    }
    DCHECK(owner_browser);
    base_window = owner_browser->window();
    web_contents = owner_browser->tab_strip_model()->GetActiveWebContents();
  }

  DCHECK(base_window);
  DCHECK(web_contents);
  profile_ = Profile::FromBrowserContext(web_contents->GetBrowserContext());
  DCHECK(profile_);

  // Check if we have another dialog opened for the contents. It's unlikely, but
  // possible. In such situation, discard this request.
  RoutingID routing_id = GetRoutingIDFromWebContents(web_contents);
  if (PendingExists(routing_id))
    return;

  const PrefService* pref_service = profile_->GetPrefs();
  DCHECK(pref_service);

  base::FilePath download_default_path(
      pref_service->GetFilePath(prefs::kDownloadDefaultDirectory));

  base::FilePath selection_path = default_path.IsAbsolute() ?
      default_path : download_default_path.Append(default_path.BaseName());

  base::FilePath fallback_path = profile_->last_selected_directory().empty() ?
      download_default_path : profile_->last_selected_directory();

  // Convert the above absolute paths to virtual paths.
  // TODO(mtomasz): Use URLs instead of paths.
  base::FilePath selection_virtual_path;
  if (!file_manager::util::ConvertAbsoluteFilePathToRelativeFileSystemPath(
      profile_,
      file_manager::kFileManagerAppId,
      selection_path,
      &selection_virtual_path)) {
    // Due to the current design, an invalid temporal cache file path may passed
    // as |default_path| (crbug.com/178013 #9-#11). In such a case, we use the
    // last selected directory as a workaround. Real fix is tracked at
    // crbug.com/110119.
    if (!file_manager::util::ConvertAbsoluteFilePathToRelativeFileSystemPath(
        profile_,
        file_manager::kFileManagerAppId,
        fallback_path.Append(default_path.BaseName()),
        &selection_virtual_path)) {
      LOG(ERROR) << "Unable to resolve the selection path.";
      return;
    }
  }
  // TODO(mtomasz): Adding a leading separator works, because this code is
  // executed on Chrome OS only. This trick will be removed, once migration to
  // URLs is finished.
  selection_virtual_path = base::FilePath("/").Append(selection_virtual_path);

  base::FilePath current_directory_virtual_path;
  base::FilePath current_directory_path = selection_path.DirName();
  if (!file_manager::util::ConvertAbsoluteFilePathToRelativeFileSystemPath(
      profile_,
      file_manager::kFileManagerAppId,
      current_directory_path,
      &current_directory_virtual_path)) {
    // Fallback if necessary, see the comment above.
    if (!file_manager::util::ConvertAbsoluteFilePathToRelativeFileSystemPath(
            profile_,
            file_manager::kFileManagerAppId,
            fallback_path,
            &current_directory_virtual_path)) {
      LOG(ERROR) << "Unable to resolve the current directory path: "
                 << fallback_path.value();
      return;
    }
  }
  current_directory_virtual_path = base::FilePath("/").Append(
      current_directory_virtual_path);

  has_multiple_file_type_choices_ =
      !file_types || (file_types->extensions.size() > 1);

  GURL file_manager_url =
      file_manager::util::GetFileManagerMainPageUrlWithParams(
          type,
          title,
          current_directory_virtual_path,
          selection_virtual_path,
          file_types,
          file_type_index,
          default_extension);

  ExtensionDialog* dialog = ExtensionDialog::Show(file_manager_url,
      base_window, profile_, web_contents,
      kFileManagerWidth, kFileManagerHeight,
      kFileManagerMinimumWidth,
      kFileManagerMinimumHeight,
#if defined(USE_AURA)
      file_manager::util::GetSelectFileDialogTitle(type),
#else
      // HTML-based header used.
      base::string16(),
#endif
      this /* ExtensionDialog::Observer */);
  if (!dialog) {
    LOG(ERROR) << "Unable to create extension dialog";
    return;
  }

  // Connect our listener to FileDialogFunction's per-tab callbacks.
  AddPending(routing_id);

  extension_dialog_ = dialog;
  params_ = params;
  routing_id_ = routing_id;
  owner_window_ = owner_window;
}
