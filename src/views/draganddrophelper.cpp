/*
 * SPDX-FileCopyrightText: 2007-2011 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "draganddrophelper.h"

#include <KFileItem>
#include <KIO/DropJob>
#include <KJobWidgets>
#include <KMountPoint>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>

#include <algorithm>

QHash<QUrl, bool> DragAndDropHelper::m_urlListMatchesUrlCache;

Qt::DropAction DragAndDropHelper::suggestedDropAction(const QList<QUrl> &urls, const QUrl &destUrl)
{
    if (urls.isEmpty()) {
        return Qt::CopyAction;
    }

    // Every source is already directly inside destUrl (e.g. hovering the empty
    // space of the same window/folder it's already showing). This is a
    // same-location no-op regardless of scheme, so it doesn't need a
    // mount-point lookup and must be checked before the "not local" bail-out
    // below, or remote (e.g. sftp) locations would always show Copy even when
    // hovering within the very same window. Mirrors KIO::DropJob's own
    // equalDestination check so the hover glyph and the actual (no-op) drop
    // decision agree.
    const bool equalDestination = std::all_of(urls.cbegin(), urls.cend(), [&destUrl](const QUrl &url) {
        return destUrl.matches(url.adjusted(QUrl::RemoveFilename), QUrl::StripTrailingSlash);
    });
    if (equalDestination) {
        return Qt::MoveAction;
    }

    // Conservative fallbacks: a destination we can't resolve to a local mount
    // point (remote, trash, non-URL) is never a same-device move.
    if (!destUrl.isLocalFile()) {
        return Qt::CopyAction;
    }

    const KMountPoint::List mountPoints = KMountPoint::currentMountPoints();

    const KMountPoint::Ptr destMountPoint = mountPoints.findByPath(destUrl.path());
    const QString destDevice = destMountPoint ? destMountPoint->mountedFrom() : QString();
    if (destDevice.isEmpty()) {
        // Local destination with no matching mount entry; be conservative.
        return Qt::CopyAction;
    }

    for (const QUrl &url : urls) {
        if (!url.isLocalFile()) {
            return Qt::CopyAction;
        }

        const KMountPoint::Ptr sourceMountPoint = mountPoints.findByPath(url.path());
        const QString sourceDevice = sourceMountPoint ? sourceMountPoint->mountedFrom() : QString();
        if (sourceDevice.isEmpty()) {
            // Local file we can't resolve a mount for; be conservative.
            return Qt::CopyAction;
        }

        // A symlink crossing to a different device is still a "move" in the
        // user's mental model; treat it as same-device like KIO::DropJob does.
        if (sourceDevice != destDevice && !KFileItem(url).isLink()) {
            return Qt::CopyAction;
        }
    }

    return Qt::MoveAction;
}

bool DragAndDropHelper::urlListMatchesUrl(const QList<QUrl> &urls, const QUrl &destUrl)
{
    auto iteratorResult = m_urlListMatchesUrlCache.constFind(destUrl);
    if (iteratorResult != m_urlListMatchesUrlCache.constEnd()) {
        return *iteratorResult;
    }

    const bool destUrlMatches = std::find_if(urls.constBegin(),
                                             urls.constEnd(),
                                             [destUrl](const QUrl &url) {
                                                 return url.matches(destUrl, QUrl::StripTrailingSlash);
                                             })
        != urls.constEnd();

    return *m_urlListMatchesUrlCache.insert(destUrl, destUrlMatches);
}

KIO::DropJob *DragAndDropHelper::dropUrls(const QUrl &destUrl, QDropEvent *event, QWidget *window, KIO::DropJobFlags dropjobFlags)
{
    const QMimeData *mimeData = event->mimeData();
    if (isArkDndMimeType(mimeData)) {
        const QString remoteDBusClient = QString::fromUtf8(mimeData->data(arkDndServiceMimeType()));
        const QString remoteDBusPath = QString::fromUtf8(mimeData->data(arkDndPathMimeType()));

        QDBusMessage message = QDBusMessage::createMethodCall(remoteDBusClient,
                                                              remoteDBusPath,
                                                              QStringLiteral("org.kde.ark.DndExtract"),
                                                              QStringLiteral("extractSelectedFilesTo"));
        message.setArguments({destUrl.toDisplayString(QUrl::PreferLocalFile)});
        QDBusConnection::sessionBus().call(message);
    } else {
        if (urlListMatchesUrl(event->mimeData()->urls(), destUrl)) {
            return nullptr;
        }

        // Drop into a directory or a desktop-file
        KIO::DropJob *job = KIO::drop(event, destUrl, dropjobFlags);
        KJobWidgets::setWindow(job, window);
        return job;
    }

    return nullptr;
}

bool DragAndDropHelper::supportsDropping(const KFileItem &destItem)
{
    return (destItem.isDir() && destItem.isWritable()) || destItem.isDesktopFile() || (destItem.isFile() && destItem.isLocalFile() && destItem.isExecutable());
}

void DragAndDropHelper::updateDropAction(QDropEvent *event, const QUrl &destUrl)
{
    if (urlListMatchesUrl(event->mimeData()->urls(), destUrl)) {
        event->setDropAction(Qt::IgnoreAction);
        event->ignore();
    }
    KFileItem item(destUrl);
    if (!item.isLocalFile() || supportsDropping(item)) {
        event->setDropAction(event->proposedAction());
        event->accept();
    } else {
        event->setDropAction(Qt::IgnoreAction);
        event->ignore();
    }
}

void DragAndDropHelper::clearUrlListMatchesUrlCache()
{
    DragAndDropHelper::m_urlListMatchesUrlCache.clear();
}

bool DragAndDropHelper::isArkDndMimeType(const QMimeData *mimeData)
{
    return mimeData->hasFormat(arkDndServiceMimeType()) && mimeData->hasFormat(arkDndPathMimeType());
}
