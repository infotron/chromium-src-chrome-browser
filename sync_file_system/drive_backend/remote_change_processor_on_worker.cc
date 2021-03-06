// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync_file_system/drive_backend/remote_change_processor_on_worker.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "chrome/browser/sync_file_system/drive_backend/callback_helper.h"
#include "chrome/browser/sync_file_system/drive_backend/remote_change_processor_wrapper.h"
#include "chrome/browser/sync_file_system/file_change.h"
#include "chrome/browser/sync_file_system/sync_file_metadata.h"

namespace sync_file_system {
namespace drive_backend {

RemoteChangeProcessorOnWorker::RemoteChangeProcessorOnWorker(
      const base::WeakPtr<RemoteChangeProcessorWrapper>& wrapper,
      base::SingleThreadTaskRunner* ui_task_runner,
      base::SequencedTaskRunner* worker_task_runner)
    : wrapper_(wrapper),
      ui_task_runner_(ui_task_runner),
      worker_task_runner_(worker_task_runner) {}

RemoteChangeProcessorOnWorker::~RemoteChangeProcessorOnWorker() {}

void RemoteChangeProcessorOnWorker::PrepareForProcessRemoteChange(
    const fileapi::FileSystemURL& url,
    const PrepareChangeCallback& callback) {
  ui_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&RemoteChangeProcessorWrapper::PrepareForProcessRemoteChange,
                 wrapper_,
                 url,
                 RelayCallbackToTaskRunner(
                     worker_task_runner_,
                     FROM_HERE,
                     callback)));
}

void RemoteChangeProcessorOnWorker::ApplyRemoteChange(
    const FileChange& change,
    const base::FilePath& local_path,
    const fileapi::FileSystemURL& url,
    const SyncStatusCallback& callback) {
  ui_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&RemoteChangeProcessorWrapper::ApplyRemoteChange,
                 wrapper_,
                 change,
                 local_path,
                 url,
                 RelayCallbackToTaskRunner(
                     worker_task_runner_,
                     FROM_HERE,
                     callback)));
}

void RemoteChangeProcessorOnWorker::FinalizeRemoteSync(
    const fileapi::FileSystemURL& url,
    bool clear_local_changes,
    const base::Closure& completion_callback) {
  ui_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&RemoteChangeProcessorWrapper::FinalizeRemoteSync,
                 wrapper_,
                 url,
                 clear_local_changes,
                 RelayCallbackToTaskRunner(
                     worker_task_runner_,
                     FROM_HERE,
                     completion_callback)));
}

void RemoteChangeProcessorOnWorker::RecordFakeLocalChange(
    const fileapi::FileSystemURL& url,
    const FileChange& change,
    const SyncStatusCallback& callback) {
  ui_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&RemoteChangeProcessorWrapper::RecordFakeLocalChange,
                 wrapper_,
                 url,
                 change,
                 RelayCallbackToTaskRunner(
                     worker_task_runner_,
                     FROM_HERE,
                     callback)));
}

}  // namespace drive_backend
}  // namespace sync_file_system
