/*
  quickinspectorwidget.cpp

  This file is part of GammaRay, the Qt application inspection and
  manipulation tool.

  Copyright (C) 2014 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Volker Krause <volker.krause@kdab.com>

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

#include "quickinspectorwidget.h"
#include "quickinspectorclient.h"
#include "quickclientitemmodel.h"
#include "quickitemtreewatcher.h"
#include "ui_quickinspectorwidget.h"

#include <common/objectbroker.h>
#include <ui/deferredresizemodesetter.h>

#include <QLabel>
#include <QTimer>

using namespace GammaRay;


static QObject* createQuickInspectorClient(const QString &/*name*/, QObject *parent)
{
  return new QuickInspectorClient(parent);
}

QuickInspectorWidget::QuickInspectorWidget(QWidget* parent) :
  QWidget(parent),
  ui(new Ui::QuickInspectorWidget),
  m_renderTimer(new QTimer(this)),
  m_sceneChangedSinceLastRequest(false),
  m_waitingForImage(false)
{
  ObjectBroker::registerClientObjectFactoryCallback<QuickInspectorInterface*>(createQuickInspectorClient);
  m_interface = ObjectBroker::object<QuickInspectorInterface*>();
  connect(m_interface, SIGNAL(sceneChanged()), this, SLOT(sceneChanged()));
  connect(m_interface, SIGNAL(sceneRendered(QImage)), this, SLOT(sceneRendered(QImage)));

  ui->setupUi(this);

  ui->windowComboBox->setModel(ObjectBroker::model("com.kdab.GammaRay.QuickWindowModel"));
  connect(ui->windowComboBox, SIGNAL(currentIndexChanged(int)), m_interface, SLOT(selectWindow(int)));
  if (ui->windowComboBox->currentIndex() >= 0)
    m_interface->selectWindow(ui->windowComboBox->currentIndex());

  QSortFilterProxyModel *proxy = new QuickClientItemModel(this);
  proxy->setSourceModel(ObjectBroker::model("com.kdab.GammaRay.QuickItemModel"));
  proxy->setDynamicSortFilter(true);
  ui->itemTreeView->setModel(proxy);
  ui->itemTreeSearchLine->setProxy(proxy);
  QItemSelectionModel* selectionModel = ObjectBroker::selectionModel(proxy);
  ui->itemTreeView->setSelectionModel(selectionModel);
  connect(selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          this, SLOT(itemSelectionChanged(QItemSelection)));

  QSortFilterProxyModel *sgProxy = new QSortFilterProxyModel(this);
  sgProxy->setSourceModel(ObjectBroker::model("com.kdab.GammaRay.QuickSceneGraphModel"));
  sgProxy->setDynamicSortFilter(true);
  ui->sgTreeView->setModel(sgProxy);
  ui->sgTreeSearchLine->setProxy(sgProxy);
  QItemSelectionModel* sgSelectionModel = ObjectBroker::selectionModel(sgProxy);
  ui->sgTreeView->setSelectionModel(sgSelectionModel);
  connect(sgSelectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          this, SLOT(itemSelectionChanged(QItemSelection)));

  new QuickItemTreeWatcher(ui->itemTreeView);
  new DeferredResizeModeSetter(ui->itemTreeView->header(), 0, QHeaderView::ResizeToContents);

  ui->itemPropertyWidget->setObjectBaseName("com.kdab.GammaRay.QuickItem");
  ui->sgPropertyWidget->setObjectBaseName("com.kdab.GammaRay.QuickSceneGraph");

  m_sceneImage = new QLabel;
  ui->sceneView->setWidget(m_sceneImage);
  ui->sceneView->setBackgroundRole(QPalette::Dark);

  m_renderTimer->setInterval(100);
  m_renderTimer->setSingleShot(true);
  connect(m_renderTimer, SIGNAL(timeout()), this, SLOT(requestRender()));
  m_interface->renderScene();
}

QuickInspectorWidget::~QuickInspectorWidget()
{
}

void QuickInspectorWidget::sceneChanged()
{
  if (!m_renderTimer->isActive())
    m_renderTimer->start();
}

void QuickInspectorWidget::sceneRendered(const QImage& img)
{
  m_waitingForImage = false;

  m_sceneImage->resize(img.size());
  m_sceneImage->setPixmap(QPixmap::fromImage(img));

  if (m_sceneChangedSinceLastRequest) {
    m_sceneChangedSinceLastRequest = false;
    sceneChanged();
  }
}

void QuickInspectorWidget::requestRender()
{
  if (!m_waitingForImage) {
    m_waitingForImage = true;
    m_interface->renderScene();
  } else {
    m_sceneChangedSinceLastRequest = true;
  }
}

void QuickInspectorWidget::itemSelectionChanged(const QItemSelection& selection)
{
  if (selection.isEmpty())
    return;
  const QModelIndex &index = selection.first().topLeft();
  ui->itemTreeView->scrollTo(index);
}

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
Q_EXPORT_PLUGIN(QuickInspectorUiFactory)
#endif
