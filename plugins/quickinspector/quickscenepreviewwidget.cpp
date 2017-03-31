/*
  quickscenepreviewwidget.cpp

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

#include "quickscenepreviewwidget.h"
#include "quickinspectorinterface.h"
#include "quickoverlaylegend.h"
#include "gridsettingswidget.h"

#include <common/streamoperators.h>

#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QWidgetAction>
#include <QAction>
#include <QMenu>
#include <QComboBox>
#include <QLabel>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

#include <cmath>

using namespace GammaRay;
static qint32 QuickScenePreviewWidgetStateVersion = 2;

QT_BEGIN_NAMESPACE
GAMMARAY_ENUM_STREAM_OPERATORS(GammaRay::QuickInspectorInterface::RenderMode)
QT_END_NAMESPACE


QuickScenePreview::QuickScenePreview(QuickInspectorInterface *inspector, QWidget *parent)
    : QWidget (parent)
    , m_toolBar(new QToolBar(this))
    , m_previewWidget(new QuickScenePreviewWidget(inspector, this))
    , m_inspectorInterface(inspector)
    , m_gridSettingsWidget(new GridSettingsWidget(this))
    , m_legendTool(new QuickOverlayLegend(this))
{
    setLayout(new QVBoxLayout());

    m_toolBarContent.visualizeGroup = new QActionGroup(this);
    m_toolBarContent.visualizeGroup->setExclusive(false); // we need 0 or 1 selected, not exactly 1

    m_toolBarContent.visualizeClipping
        = new QAction(QIcon(QStringLiteral(
                                ":/gammaray/plugins/quickinspector/visualize-clipping.png")),
                      tr("Visualize Clipping"),
                      this);
    m_toolBarContent.visualizeClipping->setActionGroup(m_toolBarContent.visualizeGroup);
    m_toolBarContent.visualizeClipping->setCheckable(true);
    m_toolBarContent.visualizeClipping->setToolTip(tr("<b>Visualize Clipping</b><br/>"
                                               "Items with the property <i>clip</i> set to true, will cut off their and their "
                                               "children's rendering at the items' bounds. While this is a handy feature it "
                                               "comes with quite some cost, like disabling some performance optimizations.<br/>"
                                               "With this tool enabled the QtQuick renderer highlights items, that have clipping "
                                               "enabled, so you can check for items, that have clipping enabled unnecessarily. "));

    m_toolBarContent.visualizeOverdraw
        = new QAction(QIcon(QStringLiteral(
                                ":/gammaray/plugins/quickinspector/visualize-overdraw.png")),
                      tr("Visualize Overdraw"),
                      this);
    m_toolBarContent.visualizeOverdraw->setActionGroup(m_toolBarContent.visualizeGroup);
    m_toolBarContent.visualizeOverdraw->setCheckable(true);
    m_toolBarContent.visualizeOverdraw->setToolTip(tr("<b>Visualize Overdraw</b><br/>"
                                               "The QtQuick renderer doesn't detect if an item is obscured by another "
                                               "opaque item, is completely outside the scene or outside a clipped ancestor and "
                                               "thus doesn't need to be rendered. You thus need to take care of setting "
                                               "<i>visible: false</i> for hidden items, yourself.<br/>"
                                               "With this tool enabled the QtQuick renderer draws a 3D-Box visualizing the "
                                               "layers of items that are drawn."));

    m_toolBarContent.visualizeBatches
        = new QAction(QIcon(QStringLiteral(
                                ":/gammaray/plugins/quickinspector/visualize-batches.png")),
                      tr("Visualize Batches"), this);
    m_toolBarContent.visualizeBatches->setActionGroup(m_toolBarContent.visualizeGroup);
    m_toolBarContent.visualizeBatches->setCheckable(true);
    m_toolBarContent.visualizeBatches->setToolTip(tr("<b>Visualize Batches</b><br/>"
                                              "Where a traditional 2D API, such as QPainter, Cairo or Context2D, is written to "
                                              "handle thousands of individual draw calls per frame, OpenGL is a pure hardware "
                                              "API and performs best when the number of draw calls is very low and state "
                                              "changes are kept to a minimum. Therefore the QtQuick renderer combines the "
                                              "rendering of similar items into single batches.<br/>"
                                              "Some settings (like <i>clip: true</i>) will cause the batching to fail, though, "
                                              "causing items to be rendered separately. With this tool enabled the QtQuick "
                                              "renderer visualizes those batches, by drawing all items that are batched using "
                                              "the same color. The fewer colors you see in this mode the better."));

    m_toolBarContent.visualizeChanges
        = new QAction(QIcon(QStringLiteral(
                                ":/gammaray/plugins/quickinspector/visualize-changes.png")),
                      tr("Visualize Changes"), this);
    m_toolBarContent.visualizeChanges->setActionGroup(m_toolBarContent.visualizeGroup);
    m_toolBarContent.visualizeChanges->setCheckable(true);
    m_toolBarContent.visualizeChanges->setToolTip(tr("<b>Visualize Changes</b><br>"
                                              "The QtQuick scene is only repainted, if some item changes in a visual manner. "
                                              "Unnecessary repaints can have a bad impact on the performance. With this tool "
                                              "enabled, the QtQuick renderer will thus on each repaint highlight the item(s), "
                                              "that caused the repaint."));

    m_toolBarContent.serverSideDecorationsEnabled = new QAction(QIcon(QStringLiteral(
                                                               ":/gammaray/plugins/quickinspector/active-focus.png")),
                                                     tr("Target Decorations"), this);
    m_toolBarContent.serverSideDecorationsEnabled->setCheckable(true);
    m_toolBarContent.serverSideDecorationsEnabled->setToolTip(tr("<b>Target Decorations</b><br>"
                                              "This enable or not the decorations on the target application."));

    QWidgetAction *gridSettingsAction = new QWidgetAction(this);
    gridSettingsAction->setDefaultWidget(m_gridSettingsWidget);

    m_toolBarContent.gridSettings = new QMenu(tr("Grid Settings"), this);
    m_toolBarContent.gridSettings->setIcon(QIcon(QStringLiteral(
                                              ":/gammaray/plugins/quickinspector/active-focus.png")));
    m_toolBarContent.gridSettings->setToolTip(tr("<b>Grid Settings</b><br>"
                                              "This popup a small widget to configure the grid settings."));
    m_toolBarContent.gridSettings->setToolTipsVisible(true);
    m_toolBarContent.gridSettings->addAction(gridSettingsAction);

    m_toolBar->addActions(m_toolBarContent.visualizeGroup->actions());
    connect(m_toolBarContent.visualizeGroup, SIGNAL(triggered(QAction*)), m_previewWidget,
            SLOT(visualizeActionTriggered(QAction*)));

    m_toolBar->addSeparator();

    foreach (auto action, m_previewWidget->interactionModeActions()->actions()) {
        m_toolBar->addAction(action);
    }
    m_toolBar->addSeparator();

    m_toolBar->addAction(m_toolBarContent.serverSideDecorationsEnabled);
    connect(m_toolBarContent.serverSideDecorationsEnabled, SIGNAL(triggered(bool)), m_previewWidget,
            SLOT(serverSideDecorationsTriggered(bool)));
    m_toolBar->addSeparator();

    m_toolBar->addAction(m_previewWidget->zoomOutAction());
    m_toolBarContent.zoomCombobox = new QComboBox(this);
    m_toolBarContent.zoomCombobox->setModel(m_previewWidget->zoomLevelModel());
    connect(m_toolBarContent.zoomCombobox, SIGNAL(currentIndexChanged(int)), m_previewWidget,
            SLOT(setZoomLevel(int)));
    connect(m_previewWidget, &RemoteViewWidget::zoomLevelChanged, m_toolBarContent.zoomCombobox,
            &QComboBox::setCurrentIndex);
    m_toolBarContent.zoomCombobox->setCurrentIndex(m_previewWidget->zoomLevelIndex());

    m_toolBar->addWidget(m_toolBarContent.zoomCombobox);
    m_toolBar->addAction(m_previewWidget->zoomInAction());

    m_toolBar->addSeparator();
    m_toolBar->addAction(m_legendTool->visibilityAction());
    m_toolBar->addAction(m_toolBarContent.gridSettings->menuAction());

    QToolButton *b = qobject_cast<QToolButton *>(m_toolBar->widgetForAction(m_toolBarContent.gridSettings->menuAction()));
    b->setPopupMode(QToolButton::InstantPopup);

    connect(m_gridSettingsWidget, SIGNAL(offsetChanged(QPoint)), this, SLOT(gridOffsetChanged(QPoint)));
    connect(m_gridSettingsWidget, SIGNAL(cellSizeChanged(QSize)), this, SLOT(gridCellSizeChanged(QSize)));

    m_previewWidget->setUnavailableText(tr(
                           "No remote view available.\n(This happens e.g. when the window is minimized or the scene is hidden)"));

    setMinimumWidth(std::max(minimumWidth(), m_toolBar->sizeHint().width()));
    layout()->addWidget(m_toolBar);
    layout()->addWidget(m_previewWidget);
}

QuickScenePreview::~QuickScenePreview()
{

}

QuickScenePreviewWidget *QuickScenePreview::previewWidget() const
{
    return m_previewWidget;
}

void QuickScenePreview::setPreviewWidget(QuickScenePreviewWidget *previewWidget)
{
    m_previewWidget = previewWidget;
}

QuickScenePreviewWidget::QuickScenePreviewWidget(QuickInspectorInterface *inspector,
                                                 QWidget *parent)
    : RemoteViewWidget(parent)

    , m_inspectorInterface(inspector)
{
    setName(QStringLiteral("com.kdab.GammaRay.QuickRemoteView"));
}

QuickScenePreviewWidget::~QuickScenePreviewWidget()
{
}

void QuickScenePreviewWidget::restoreState(const QByteArray &state)
{
//    if (state.isEmpty())
//        return;

//    QDataStream stream(state);
//    qint32 version;
//    QuickInspectorInterface::RenderMode mode = customRenderMode();
//    bool drawDecorations = serverSideDecorationsEnabled();
//    RemoteViewWidget::restoreState(stream);

//    stream >> version;

//    switch (version) {
//    case 1: {
//        stream
//                >> mode
//        ;
//        break;
//    }
//    case 2: {
//        stream
//                >> mode
//                >> drawDecorations
//        ;
//        break;
//    }
//    }

//    setCustomRenderMode(mode);
//    setServerSideDecorationsEnabled(drawDecorations);
}

//QByteArray QuickScenePreviewWidget::saveState() const
//{
//    QByteArray data;

//    {
//        QDataStream stream(&data, QIODevice::WriteOnly);
//        RemoteViewWidget::saveState(stream);

//        stream << QuickScenePreviewWidgetStateVersion;

//        switch (QuickScenePreviewWidgetStateVersion) {
//        case 1: {
//            stream
//                    << customRenderMode()
//            ;
//            break;
//        }
//        case 2: {
//            stream
//                    << customRenderMode()
//                    << serverSideDecorationsEnabled()
//            ;
//            break;
//        }
//        }
//    }

//    return data;
//}

void QuickScenePreview::resizeEvent(QResizeEvent *e)
{
    m_toolBar->setGeometry(0, 0, width(),
                                         m_toolBar->sizeHint().height());
    m_previewWidget->resizeEvent(e);
}

void QuickScenePreviewWidget::drawDecoration(QPainter *p)
{
    // scaled and translated
    auto itemGeometry = frame().data().value<QuickItemGeometry>();
    itemGeometry.scaleTo(zoom());
    QuickOverlay::drawDecoration(p, QuickOverlay::RenderInfo(m_overlaySettings, itemGeometry, frame().viewRect(), zoom()));
}

void QuickScenePreview::visualizeActionTriggered(QAction *current)
{
    if (!current->isChecked()) {
        m_inspectorInterface->setCustomRenderMode(QuickInspectorInterface::NormalRendering);
    } else {
        // QActionGroup requires exactly one selected, but we need 0 or 1 selected
        foreach (auto action, m_toolBarContent.visualizeGroup->actions()) {
            if (action != current)
                action->setChecked(false);
        }
        m_inspectorInterface->setCustomRenderMode(current == m_toolBarContent.visualizeClipping ? QuickInspectorInterface::VisualizeClipping
                                                  : current == m_toolBarContent.visualizeBatches ? QuickInspectorInterface::VisualizeBatches
                                                  : current == m_toolBarContent.visualizeOverdraw ? QuickInspectorInterface::VisualizeOverdraw
                                                  : current == m_toolBarContent.visualizeChanges ? QuickInspectorInterface::VisualizeChanges
                                                  : QuickInspectorInterface::NormalRendering
                                                  );
    }
    emit m_previewWidget->stateChanged();
}

void QuickScenePreview::serverSideDecorationsTriggered(bool enabled)
{
    m_toolBarContent.serverSideDecorationsEnabled->setChecked(enabled);
    m_inspectorInterface->setServerSideDecorationsEnabled(enabled);
    emit m_previewWidget->stateChanged();
}

void QuickScenePreview::gridOffsetChanged(const QPoint &value)
{
    m_previewWidget->m_overlaySettings.gridOffset = value;
    m_legendTool->setOverlaySettings(m_previewWidget->m_overlaySettings);
    update();
    m_previewWidget->setOverlaySettings(m_previewWidget->m_overlaySettings);
}

void QuickScenePreview::gridCellSizeChanged(const QSize &value)
{
    m_previewWidget->m_overlaySettings.gridCellSize = value;
    m_legendTool->setOverlaySettings(m_previewWidget->m_overlaySettings);
    update();
    m_previewWidget->setOverlaySettings(m_previewWidget->m_overlaySettings);
}

void GammaRay::QuickScenePreview::setSupportsCustomRenderModes(
    QuickInspectorInterface::Features supportedCustomRenderModes)
{
    m_toolBarContent.visualizeClipping->setEnabled(
        supportedCustomRenderModes & QuickInspectorInterface::CustomRenderModeClipping);
    m_toolBarContent.visualizeBatches->setEnabled(
        supportedCustomRenderModes & QuickInspectorInterface::CustomRenderModeBatches);
    m_toolBarContent.visualizeOverdraw->setEnabled(
        supportedCustomRenderModes & QuickInspectorInterface::CustomRenderModeOverdraw);
    m_toolBarContent.visualizeChanges->setEnabled(
        supportedCustomRenderModes & QuickInspectorInterface::CustomRenderModeChanges);
}

void QuickScenePreview::setServerSideDecorationsState(bool enabled)
{
    m_toolBarContent.serverSideDecorationsEnabled->setChecked(enabled);
}

void QuickScenePreview::setOverlaySettingsState(const QuickOverlaySettings &settings)
{
    m_previewWidget->m_overlaySettings = settings;
    m_gridSettingsWidget->setOverlaySettings(settings);
    m_legendTool->setOverlaySettings(settings);
}

QuickInspectorInterface::RenderMode QuickScenePreview::customRenderMode() const
{
    if (m_toolBarContent.visualizeClipping->isChecked()) {
        return QuickInspectorInterface::VisualizeClipping;
    }
    else if (m_toolBarContent.visualizeBatches->isChecked()) {
        return QuickInspectorInterface::VisualizeBatches;
    }
    else if (m_toolBarContent.visualizeOverdraw->isChecked()) {
        return QuickInspectorInterface::VisualizeOverdraw;
    }
    else if (m_toolBarContent.visualizeChanges->isChecked()) {
        return QuickInspectorInterface::VisualizeChanges;
    }

    return QuickInspectorInterface::NormalRendering;
}

void QuickScenePreview::setCustomRenderMode(QuickInspectorInterface::RenderMode customRenderMode)
{
    if (this->customRenderMode() == customRenderMode)
        return;

    QAction *currentAction = nullptr;
    switch (customRenderMode) {
    case QuickInspectorInterface::NormalRendering:
        break;
    case QuickInspectorInterface::VisualizeClipping:
        currentAction = m_toolBarContent.visualizeClipping;
        break;
    case QuickInspectorInterface::VisualizeOverdraw:
        currentAction = m_toolBarContent.visualizeOverdraw;
        break;
    case QuickInspectorInterface::VisualizeBatches:
        currentAction = m_toolBarContent.visualizeBatches;
        break;
    case QuickInspectorInterface::VisualizeChanges:
        currentAction = m_toolBarContent.visualizeChanges;
        break;
    }

    foreach (auto action, m_toolBarContent.visualizeGroup->actions()) {
        if (action)
            action->setChecked(currentAction == action);
    }

    visualizeActionTriggered(currentAction ? currentAction : m_toolBarContent.visualizeBatches);
}

QuickOverlaySettings QuickScenePreviewWidget::overlaySettings() const
{
    return m_overlaySettings;
}

void QuickScenePreviewWidget::setOverlaySettings(const QuickOverlaySettings &settings)
{
    m_inspectorInterface->setOverlaySettings(settings);
    emit stateChanged();
}

bool QuickScenePreview::serverSideDecorationsEnabled() const
{
    return m_toolBarContent.serverSideDecorationsEnabled->isChecked();
}

void QuickScenePreview::setServerSideDecorationsEnabled(bool enabled)
{
    if (m_toolBarContent.serverSideDecorationsEnabled->isChecked() == enabled)
        return;
    m_toolBarContent.serverSideDecorationsEnabled->setChecked(enabled);
    serverSideDecorationsTriggered(enabled);
}
