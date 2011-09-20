// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_DBUS_PROXY_RESOLUTION_SERVICE_PROVIDER_H_
#define CHROME_BROWSER_CHROMEOS_DBUS_PROXY_RESOLUTION_SERVICE_PROVIDER_H_
#pragma once

#include <string>

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/synchronization/lock.h"
#include "base/threading/platform_thread.h"
#include "chrome/browser/chromeos/dbus/cros_dbus_service.h"

namespace dbus {
class ExportedObject;
class MethodCall;
class Response;
}

namespace chromeos {

class ProxyResolverInterface;

// This class provides proxy resolution service for CrosDBusService.
// It processes proxy resolution requests for ChromeOS clients.
//
// The following methods are exported.
//
// Interface: org.chromium.LibCrosServiceInterface (kLibCrosServiceInterface)
// Method: ResolveNetworkProxy (kResolveNetworkProxy)
// Parameters: string:source_url
//             string:signal_interface
//             string:signal_name
//
//   Resolves the proxy for |source_url|. Returns the result
//   as a D-Bus signal sent to |signal_interface| and |signal_name|.
//
//   The returned signal will contain the three values:
//   - string:source_url - requested source URL.
//   - string:proxy_info - proxy info for the source URL in PAC format
//                         like "PROXY cache.example.com:12345"
//   - string:error_message - error message. Empty if successful.
//
class ProxyResolutionServiceProvider
    : public CrosDBusService::ServiceProviderInterface {
 public:
  virtual ~ProxyResolutionServiceProvider();

  // CrosDBusService::ServiceProviderInterface override.
  virtual void Start(scoped_refptr<dbus::ExportedObject> exported_object);

  // Gets the instance.
  static ProxyResolutionServiceProvider* Get();

 private:
  explicit ProxyResolutionServiceProvider(ProxyResolverInterface *resovler);

  // Gets the instance for testing. Takes the ownership of |resovler|
  friend class ProxyResolutionServiceProviderTest;
  static ProxyResolutionServiceProvider* GetForTesting(
      ProxyResolverInterface* resolver);

  // Called from ExportedObject, when ResolveProxyHandler() is exported as
  // a D-Bus method, or failed to be exported.
  void OnExported(const std::string& interface_name,
                  const std::string& method_name,
                  bool success);

  // Callback to be invoked when ChromeOS clients send network proxy
  // resolution requests to the service running in chrome executable.
  // Called on UI thread from dbus request.
  dbus::Response* ResolveProxyHandler(dbus::MethodCall* method_call);

  // Returns true if the current thread is on the origin thread.
  bool OnOriginThread();

  scoped_refptr<dbus::ExportedObject> exported_object_;
  scoped_refptr<ProxyResolverInterface> resolver_;
  base::PlatformThreadId origin_thread_id_;

  DISALLOW_COPY_AND_ASSIGN(ProxyResolutionServiceProvider);
};

// The interface is defined so we can mock out the proxy resolver
// implementation.
//
// ProxyResolverInterface is a ref counted object, to ensure that |this|
// of the object is alive when callbacks referencing |this| are called.
class ProxyResolverInterface
    : public base::RefCountedThreadSafe<ProxyResolverInterface> {
 public:
  virtual ~ProxyResolverInterface();

  // Resolves the proxy for the given URL. Returns the result as a
  // signal sent to |signal_interface| and
  // |signal_name|. |exported_object| will be used to send the
  // signal. The signal contains the three string members:
  //
  // - source url: the requested source URL.
  // - proxy info: proxy info for the source URL in PAC format.
  // - error message: empty if the proxy resolution was successful.
  virtual void ResolveProxy(
      const std::string& source_url,
      const std::string& signal_interface,
      const std::string& signal_name,
      scoped_refptr<dbus::ExportedObject> exported_object) = 0;

 private:
  friend class base::RefCountedThreadSafe<ProxyResolverInterface>;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_DBUS_PROXY_RESOLUTION_SERVICE_PROVIDER_H_
