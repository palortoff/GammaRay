/*
  quickscenegraphmodel.cpp

  This file is part of GammaRay, the Qt application inspection and
  manipulation tool.

  Copyright (C) 2014 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Anton Kreuzkamp <anton.kreuzkamp@kdab.com>

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

#include "quickscenegraphmodel.h"

#ifdef HAVE_PRIVATE_QT_HEADERS

#include <private/qquickitem_p.h> //krazy:exclude=camelcase
#include "quickitemmodelroles.h"

#include <QDebug>
#include <QQuickWindow>
#include <QThread>
#include <QSGNode>

#include <algorithm>

Q_DECLARE_METATYPE(QSGNode*)

using namespace GammaRay;

QuickSceneGraphModel::QuickSceneGraphModel(QObject* parent) : ObjectModelBase<QAbstractItemModel>(parent)
{
}

QuickSceneGraphModel::~QuickSceneGraphModel()
{
}

void QuickSceneGraphModel::setWindow(QQuickWindow* window)
{
  beginResetModel();
  clear();
  m_window = window;
  QQuickItem *item = window->contentItem();
  QQuickItemPrivate *itemPriv = QQuickItemPrivate::get(item);
  m_rootNode = itemPriv->itemNode();
  while(m_rootNode->parent()) // Ensure that we really get the very root node.
      m_rootNode = m_rootNode->parent();
  updateSGTree();
  connect(window, SIGNAL(beforeRendering()), this, SLOT(updateSGTree()));

  endResetModel();
}

void QuickSceneGraphModel::updateSGTree()
{
  QQuickItem *item = m_window->contentItem();
  QQuickItemPrivate *itemPriv = QQuickItemPrivate::get(item);
  QSGNode *rootNode = itemPriv->itemNode();
  while(rootNode->parent()) // Ensure that we really get the very root node.
      rootNode = rootNode->parent();
  m_oldChildParentMap = m_childParentMap;
  m_oldParentChildMap = m_parentChildMap;
  m_childParentMap = QHash<QSGNode*, QSGNode*>();
  m_parentChildMap = QHash<QSGNode*, QVector<QSGNode*> >();
  m_childParentMap[m_rootNode] = 0;
  m_parentChildMap[0].append(m_rootNode);

  populateFromNode(m_rootNode);
  collectItemNodes(m_window->contentItem());
}

QVariant QuickSceneGraphModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid())
    return QVariant();

  QSGNode *node = reinterpret_cast<QSGNode*>(index.internalPointer());


  if (role == Qt::DisplayRole) {
    if (index.column() == 0) {
      return Util::addressToString(node);
    } else if (index.column() == 1) {
      switch (node->type()) {
      case QSGNode::BasicNodeType:
        return "Node";
      case QSGNode::GeometryNodeType:
        return "Geometry Node";
      case QSGNode::TransformNodeType:
        return "Transform Node";
      case QSGNode::ClipNodeType:
        return "Clip Node";
      case QSGNode::OpacityNodeType:
        return "Opacity Node";
      case QSGNode::RootNodeType:
        return "Root Node";
      case QSGNode::RenderNodeType:
        return "Render Node";
      }
    }
  } else if (role == ObjectModel::ObjectRole) {
    return QVariant::fromValue(node);
  }

  return QVariant();
}

int QuickSceneGraphModel::rowCount(const QModelIndex& parent) const
{
  if (parent.column() == 1) {
    return 0;
  }
  QSGNode *parentNode = reinterpret_cast<QSGNode*>(parent.internalPointer());
  return m_parentChildMap.value(parentNode).size();
}

QModelIndex QuickSceneGraphModel::parent(const QModelIndex& child) const
{
  QSGNode *childNode = reinterpret_cast<QSGNode*>(child.internalPointer());
  return indexForNode(m_childParentMap.value(childNode));
}

QModelIndex QuickSceneGraphModel::index(int row, int column, const QModelIndex& parent) const
{
  QSGNode *parentNode = reinterpret_cast<QSGNode*>(parent.internalPointer());
  const QVector<QSGNode*> children = m_parentChildMap.value(parentNode);
  if (row < 0 || column < 0 || row >= children.size()  || column >= columnCount()) {
    return QModelIndex();
  }
  return createIndex(row, column, children.at(row));
}

void QuickSceneGraphModel::clear()
{
  m_childParentMap.clear();
  m_parentChildMap.clear();
}

void QuickSceneGraphModel::populateFromNode(QSGNode *node)
{
  if (!node)
    return;

  QVector<QSGNode*> &childList  = m_parentChildMap[node];
  QVector<QSGNode*> &oldChildList  = m_oldParentChildMap[node];
  QVector<QSGNode*> newChildList;

  for (int i = 0; i < node->childCount(); i++) {
    QSGNode *childNode = node->childAtIndex(i);

    newChildList.append(childNode);
  }

  QModelIndex myIndex = indexForNode(node);

  std::sort(newChildList.begin(), newChildList.end());

  QVector<QSGNode*>::iterator i = oldChildList.begin();
  QVector<QSGNode*>::const_iterator j = newChildList.constBegin();

  while (i != oldChildList.end() && j != newChildList.constEnd()) {
    if (*i < *j) { // We don't have to do anything but inform the client about the change
      beginRemoveRows(myIndex, childList.size(), childList.size());
      endRemoveRows();
      i++;
    } else if (*i > *j) { // Add to new list and inform the client about the change
      beginInsertRows(myIndex, childList.size(), childList.size());
      m_childParentMap.insert(*j, node);
      childList.append(*j);
      endInsertRows();
      populateFromNode(*j);
      j++;
    } else { // Adopt to new list, without informing the client (as nothing really changed)
      Q_ASSERT(*i == *j);
      m_childParentMap.insert(*j, node);
      childList.append(*j);
      populateFromNode(*j);
      j++;
      i++;
    }
  }
  if (i == oldChildList.end() && j != newChildList.constEnd()) { // Add remaining new items to list and inform the client
    beginInsertRows(myIndex, childList.size(), childList.size() + std::distance(j, newChildList.constEnd()) - 1);
    for (;j != newChildList.constEnd(); j++) {
      m_childParentMap.insert(*j, node);
      childList.append(*j);
      populateFromNode(*j);
    }
    endInsertRows();
  } else if (i != oldChildList.end()) { // Inform the client about the removed rows
    beginRemoveRows(myIndex, childList.size(), childList.size() + std::distance(i, oldChildList.end()) - 1);
    endRemoveRows();
  }
}

void QuickSceneGraphModel::collectItemNodes(QQuickItem *item)
{
  if (!item)
    return;

  QSGNode *itemNode = QQuickItemPrivate::get(item)->itemNode();
  m_itemItemNodeMap[item] = itemNode;
  m_itemNodeItemMap[itemNode] = item;

  foreach (QQuickItem *child, item->childItems())
    collectItemNodes(child);
}


QModelIndex QuickSceneGraphModel::indexForNode(QSGNode* node) const
{
  if (!node)
    return QModelIndex();

  QSGNode *parent = m_childParentMap.value(node);
  const QModelIndex parentIndex = indexForNode(parent);
  if (!parentIndex.isValid() && parent) {
    return QModelIndex();
  }
  const QVector<QSGNode*> &siblings = m_parentChildMap[parent];
  QVector<QSGNode*>::const_iterator it = std::lower_bound(siblings.constBegin(), siblings.constEnd(), node);
  if (it == siblings.constEnd() || *it != node) {
    Q_ASSERT(!siblings.contains(node));
    return QModelIndex();
  }

  const int row = std::distance(siblings.constBegin(), it);
  return index(row, 0, parentIndex);
}

QSGNode *QuickSceneGraphModel::sgNodeForItem(QQuickItem *item) const
{
  return m_itemItemNodeMap[item];
}

QQuickItem *QuickSceneGraphModel::itemForSgNode(QSGNode *node) const
{
  while (node && !m_itemNodeItemMap.contains(node)) // If there's no entry for node, take its parent
    node = m_childParentMap[node];
  return m_itemNodeItemMap[node];
}

#endif // HAVE_PRIVATE_QT_HEADERS
