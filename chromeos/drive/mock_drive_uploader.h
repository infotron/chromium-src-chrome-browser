// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_DRIVE_MOCK_DRIVE_UPLOADER_H_
#define CHROME_BROWSER_CHROMEOS_DRIVE_MOCK_DRIVE_UPLOADER_H_

#include "base/memory/scoped_ptr.h"
#include "chrome/browser/google_apis/drive_uploader.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace drive {

class MockDriveUploader : public google_apis::DriveUploaderInterface {
 public:
  MockDriveUploader();
  virtual ~MockDriveUploader();

  MOCK_METHOD9(
      UploadNewFile,
      int(const GURL& upload_location,
          const FilePath& gdata_file_path,
          const FilePath& local_file_path,
          const std::string& title,
          const std::string& content_type,
          int64 content_length,
          int64 file_size,
          const google_apis::UploadCompletionCallback& completion_callback,
          const google_apis::UploaderReadyCallback& ready_callback));
  MOCK_METHOD8(
      StreamExistingFile,
      int(const GURL& upload_location,
          const FilePath& gdata_file_path,
          const FilePath& local_file_path,
          const std::string& content_type,
          int64 content_length,
          int64 file_size,
          const google_apis::UploadCompletionCallback& completion_callback,
          const google_apis::UploaderReadyCallback& ready_callback));
  MOCK_METHOD7(
      UploadExistingFile,
      int(const GURL& upload_location,
          const FilePath& gdata_file_path,
          const FilePath& local_file_path,
          const std::string& content_type,
          int64 file_size,
          const google_apis::UploadCompletionCallback& completion_callback,
          const google_apis::UploaderReadyCallback& ready_callback));
  MOCK_METHOD2(UpdateUpload, void(int upload_id,
                                  content::DownloadItem* download));
  MOCK_CONST_METHOD1(GetUploadedBytes, int64(int upload_id));
};

}  // namespace drive

#endif  // CHROME_BROWSER_CHROMEOS_DRIVE_MOCK_DRIVE_UPLOADER_H_
