/*
 * Copyright (C) 2016 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "RemoteWebInspectorUI.h"

#include "RemoteWebInspectorProxyMessages.h"
#include "RemoteWebInspectorUIMessages.h"
#include "WebCoreArgumentCoders.h"
#include "WebPage.h"
#include "WebProcess.h"
#include <WebCore/CertificateInfo.h>
#include <WebCore/Chrome.h>
#include <WebCore/DOMWrapperWorld.h>
#include <WebCore/FloatRect.h>
#include <WebCore/InspectorController.h>
#include <WebCore/Settings.h>

#if !PLATFORM(MAC) && !PLATFORM(GTK) && !PLATFORM(WIN)
#include <WebCore/NotImplemented.h>
#endif

namespace WebKit {
using namespace WebCore;

Ref<RemoteWebInspectorUI> RemoteWebInspectorUI::create(WebPage& page)
{
    return adoptRef(*new RemoteWebInspectorUI(page));
}

RemoteWebInspectorUI::RemoteWebInspectorUI(WebPage& page)
    : m_page(page)
    , m_frontendAPIDispatcher(page)
{
}

void RemoteWebInspectorUI::initialize(DebuggableInfoData&& debuggableInfo, const String& backendCommandsURL)
{
    m_debuggableInfo = WTFMove(debuggableInfo);
    m_backendCommandsURL = backendCommandsURL;

    m_page.corePage()->inspectorController().setInspectorFrontendClient(this);

    m_frontendAPIDispatcher.reset();
    m_frontendAPIDispatcher.dispatchCommand("setDockingUnavailable"_s, true);
}

void RemoteWebInspectorUI::didSave(const String& url)
{
    m_frontendAPIDispatcher.dispatchCommand("savedURL"_s, url);
}

void RemoteWebInspectorUI::didAppend(const String& url)
{
    m_frontendAPIDispatcher.dispatchCommand("appendedToURL"_s, url);
}

void RemoteWebInspectorUI::sendMessageToFrontend(const String& message)
{
    m_frontendAPIDispatcher.dispatchMessageAsync(message);
}

void RemoteWebInspectorUI::sendMessageToBackend(const String& message)
{
    WebProcess::singleton().parentProcessConnection()->send(Messages::RemoteWebInspectorProxy::SendMessageToBackend(message), m_page.identifier());
}

void RemoteWebInspectorUI::windowObjectCleared()
{
    if (m_frontendHost)
        m_frontendHost->disconnectClient();

    m_frontendHost = InspectorFrontendHost::create(this, m_page.corePage());
    m_frontendHost->addSelfToGlobalObjectInWorld(mainThreadNormalWorld());
}

void RemoteWebInspectorUI::frontendLoaded()
{
    m_frontendAPIDispatcher.frontendLoaded();

    m_frontendAPIDispatcher.dispatchCommand("setIsVisible"_s, true);

    bringToFront();
}

void RemoteWebInspectorUI::changeSheetRect(const FloatRect& rect)
{
    WebProcess::singleton().parentProcessConnection()->send(Messages::RemoteWebInspectorProxy::SetSheetRect(rect), m_page.identifier());
}

WebCore::UserInterfaceLayoutDirection RemoteWebInspectorUI::userInterfaceLayoutDirection() const
{
    return m_page.corePage()->userInterfaceLayoutDirection();
}

void RemoteWebInspectorUI::bringToFront()
{
    WebProcess::singleton().parentProcessConnection()->send(Messages::RemoteWebInspectorProxy::BringToFront(), m_page.identifier());
}

void RemoteWebInspectorUI::closeWindow()
{
    m_page.corePage()->inspectorController().setInspectorFrontendClient(nullptr);

    WebProcess::singleton().parentProcessConnection()->send(Messages::RemoteWebInspectorProxy::FrontendDidClose(), m_page.identifier());
}

void RemoteWebInspectorUI::reopen()
{
    WebProcess::singleton().parentProcessConnection()->send(Messages::RemoteWebInspectorProxy::Reopen(), m_page.identifier());
}

void RemoteWebInspectorUI::resetState()
{
    WebProcess::singleton().parentProcessConnection()->send(Messages::RemoteWebInspectorProxy::ResetState(), m_page.identifier());
}

void RemoteWebInspectorUI::openInNewTab(const String& url)
{
    WebProcess::singleton().parentProcessConnection()->send(Messages::RemoteWebInspectorProxy::OpenInNewTab(url), m_page.identifier());
}

void RemoteWebInspectorUI::save(const String& filename, const String& content, bool base64Encoded, bool forceSaveAs)
{
    WebProcess::singleton().parentProcessConnection()->send(Messages::RemoteWebInspectorProxy::Save(filename, content, base64Encoded, forceSaveAs), m_page.identifier());
}

void RemoteWebInspectorUI::append(const String& filename, const String& content)
{
    WebProcess::singleton().parentProcessConnection()->send(Messages::RemoteWebInspectorProxy::Append(filename, content), m_page.identifier());
}

void RemoteWebInspectorUI::inspectedURLChanged(const String& urlString)
{
    // Do nothing. The remote side can know if the main resource changed.
}

void RemoteWebInspectorUI::showCertificate(const CertificateInfo& certificateInfo)
{
    WebProcess::singleton().parentProcessConnection()->send(Messages::RemoteWebInspectorProxy::ShowCertificate(certificateInfo), m_page.identifier());
}

Inspector::DebuggableType RemoteWebInspectorUI::debuggableType() const
{
    return m_debuggableInfo.debuggableType;
}

String RemoteWebInspectorUI::targetPlatformName() const
{
    return m_debuggableInfo.targetPlatformName;
}

String RemoteWebInspectorUI::targetBuildVersion() const
{
    return m_debuggableInfo.targetBuildVersion;
}

String RemoteWebInspectorUI::targetProductVersion() const
{
    return m_debuggableInfo.targetProductVersion;
}

bool RemoteWebInspectorUI::targetIsSimulator() const
{
    return m_debuggableInfo.targetIsSimulator;
}

#if ENABLE(INSPECTOR_TELEMETRY)
bool RemoteWebInspectorUI::supportsDiagnosticLogging()
{
    return m_page.corePage()->settings().diagnosticLoggingEnabled();
}

void RemoteWebInspectorUI::logDiagnosticEvent(const String& eventName,  const DiagnosticLoggingClient::ValueDictionary& dictionary)
{
    m_page.corePage()->diagnosticLoggingClient().logDiagnosticMessageWithValueDictionary(eventName, "Remote Web Inspector Frontend Diagnostics"_s, dictionary, ShouldSample::No);
}

void RemoteWebInspectorUI::setDiagnosticLoggingAvailable(bool available)
{
    // Inspector's diagnostic logging client should never be used unless the page setting is also enabled.
    ASSERT(!available || supportsDiagnosticLogging());
    m_diagnosticLoggingAvailable = available;

    m_frontendAPIDispatcher.dispatchCommand("setDiagnosticLoggingAvailable"_s, m_diagnosticLoggingAvailable);
}
#endif

#if !PLATFORM(MAC) && !PLATFORM(GTK) && !PLATFORM(WIN)
String RemoteWebInspectorUI::localizedStringsURL() const
{
    notImplemented();
    return emptyString();
}
#endif // !PLATFORM(MAC) && !PLATFORM(GTK) && !PLATFORM(WIN)

} // namespace WebKit
