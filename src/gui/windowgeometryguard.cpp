/*
    Copyright (c) 2018, Lukas Holecek <hluk@email.cz>

    This file is part of CopyQ.

    CopyQ is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    CopyQ is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CopyQ.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "windowgeometryguard.h"

#include "common/appconfig.h"
#include "common/common.h"
#include "common/config.h"
#include "common/timer.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QEvent>
#include <QMoveEvent>
#include <QVariant>

namespace {

const char propertyGeometryLocked[] = "CopyQ_geometry_locked";

bool openOnCurrentScreen()
{
    return AppConfig().option<Config::open_windows_on_current_screen>();
}

} // namespace

void WindowGeometryGuard::create(QWidget *window)
{
    new WindowGeometryGuard(window);
}

bool WindowGeometryGuard::eventFilter(QObject *, QEvent *event)
{
    const QEvent::Type type = event->type();

    switch (type) {
    case QEvent::Show:
        restoreWindowGeometry();
        break;

    case QEvent::Move:
    case QEvent::Resize:
        if ( !isWindowGeometryLocked() && m_window->isVisible() )
            m_timerSaveGeometry.start();
        break;

    case QEvent::Hide:
        setGeometryGuardBlockedUntilHidden(m_window, false);
        break;

    default:
        break;
    }

    return false;
}

WindowGeometryGuard::WindowGeometryGuard(QWidget *window)
    : QObject(window)
    , m_window(window)
{
    connect( QApplication::desktop(), &QDesktopWidget::resized, this, &WindowGeometryGuard::restoreWindowGeometry );
    connect( QApplication::desktop(), &QDesktopWidget::workAreaResized, this, &WindowGeometryGuard::restoreWindowGeometry );
    initSingleShotTimer(&m_timerSaveGeometry, 250, this, &WindowGeometryGuard::saveWindowGeometry);
    initSingleShotTimer(&m_timerUnlockGeometry, 250, this, &WindowGeometryGuard::unlockWindowGeometry);

    m_window->installEventFilter(this);
    restoreWindowGeometry();
}

bool WindowGeometryGuard::isWindowGeometryLocked() const
{
    return m_window->property(propertyGeometryLocked).toBool()
        || isGeometryGuardBlockedUntilHidden(m_window);
}

bool WindowGeometryGuard::lockWindowGeometry()
{
    if ( isWindowGeometryLocked() )
        return false;

    m_window->setProperty(propertyGeometryLocked, true);
    m_timerUnlockGeometry.start();

    return true;
}

void WindowGeometryGuard::saveWindowGeometry()
{
    if ( !lockWindowGeometry() )
        return;

    ::saveWindowGeometry(m_window, openOnCurrentScreen());
    unlockWindowGeometry();
}

void WindowGeometryGuard::restoreWindowGeometry()
{
    if ( !lockWindowGeometry() )
        return;

    ::restoreWindowGeometry(m_window, openOnCurrentScreen());
    m_timerSaveGeometry.stop();
}

void WindowGeometryGuard::unlockWindowGeometry()
{
    m_window->setProperty(propertyGeometryLocked, false);
}
