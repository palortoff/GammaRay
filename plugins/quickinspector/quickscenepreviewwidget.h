/*
  quickscenepreviewwidget.h

  This file is part of GammaRay, the Qt application inspection and
  manipulation tool.

  Copyright (C) 2014-2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Anton Kreuzkamp <anton.kreuzkamp@kdab.com>

  Licensees holding valid commercial KDAB GammaRay licenses may use this file in
  accordance with GammaRay Commercial License Agreement provided with the Software.

  Contact info@kdab.com if any conditions of this licensing are not clear to you.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef GAMMARAY_QUICKINSPECTOR_QUICKSCENEPREVIEWWIDGET_H
#define GAMMARAY_QUICKINSPECTOR_QUICKSCENEPREVIEWWIDGET_H

#include "quickitemgeometry.h"
#include "quickinspectorinterface.h"
#include "quickoverlay.h"

#include <ui/remoteviewwidget.h>

QT_BEGIN_NAMESPACE
class QAction;
class QActionGroup;
class QMenu;
class QComboBox;
class QLabel;
class QToolBar;
QT_END_NAMESPACE

namespace GammaRay {
class QuickInspectorInterface;
class GridSettingsWidget;
class QuickOverlayLegend;

class QuickScenePreviewWidget;

class QuickScenePreview : public QWidget
{
    Q_OBJECT

public:
    explicit QuickScenePreview(QuickInspectorInterface *inspector, QWidget *parent = nullptr);
    ~QuickScenePreview();

private:
    QToolBar *m_toolBar;
    QuickScenePreviewWidget *m_previewWidget;
};

class QuickScenePreviewWidget : public RemoteViewWidget
{
    Q_OBJECT

public:
    explicit QuickScenePreviewWidget(QuickInspectorInterface *inspector, QWidget *parent = nullptr);
    ~QuickScenePreviewWidget();

    void restoreState(const QByteArray &state) override;
    QByteArray saveState() const override;

    void setSupportsCustomRenderModes(QuickInspectorInterface::Features supportedCustomRenderModes);
    void setServerSideDecorationsState(bool enabled);
    void setOverlaySettingsState(const QuickOverlaySettings &settings);

    QuickInspectorInterface::RenderMode customRenderMode() const;
    void setCustomRenderMode(QuickInspectorInterface::RenderMode customRenderMode);

    QuickOverlaySettings overlaySettings() const;
    void setOverlaySettings(const QuickOverlaySettings &settings);

    bool serverSideDecorationsEnabled() const;
    void setServerSideDecorationsEnabled(bool enabled);

private Q_SLOTS:
    void visualizeActionTriggered(QAction* current);
    void serverSideDecorationsTriggered(bool enabled);
    void gridOffsetChanged(const QPoint &value);
    void gridCellSizeChanged(const QSize &value);

private:
    void drawDecoration(QPainter *p) override;
    void resizeEvent(QResizeEvent *e) override;

    struct {
        QToolBar *toolbarWidget;
        QComboBox *zoomCombobox;
        QActionGroup *visualizeGroup;
        QAction *visualizeClipping;
        QAction *visualizeOverdraw;
        QAction *visualizeBatches;
        QAction *visualizeChanges;
        QAction *serverSideDecorationsEnabled;
        QMenu *gridSettings;
    } m_toolBar;

    QuickOverlaySettings m_overlaySettings;
    GridSettingsWidget *m_gridSettingsWidget;
    QuickOverlayLegend *m_legendTool;

    QuickInspectorInterface *m_inspectorInterface;
};
} // namespace GammaRay

#endif
