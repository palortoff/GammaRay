/*
  This file is part of GammaRay, the Qt application inspection and
  manipulation tool.

  Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Volker Krause <volker.krause@kdab.com>

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

#include <config-gammaray.h>

#include <plugins/quickinspector/quickinspectorinterface.h>
#include <probe/hooks.h>
#include <probe/probecreator.h>
#include <core/probe.h>
#include <common/paths.h>
#include <common/objectbroker.h>

#include <3rdparty/qt/modeltest.h>

#include <QtTest/qtest.h>

#include <QQuickView>
#include <QItemSelectionModel>

using namespace GammaRay;

class QuickInspectorTest : public QObject
{
    Q_OBJECT
private:
    void createProbe()
    {
        Paths::setRelativeRootPath(GAMMARAY_INVERSE_BIN_DIR);
        qputenv("GAMMARAY_ProbePath", Paths::currentProbePath().toUtf8());
        Hooks::installHooks();
        Probe::startupHookReceived();
        new ProbeCreator(ProbeCreator::CreateOnly);
        QTest::qWait(1); // event loop re-entry
    }

private slots:
    void testModels()
    {
        createProbe();

        // we need one view for the plugin to activate, otherwise the model will not be available
        auto view = new QQuickView;
        view->show();
        QTest::qWait(1); // event loop re-entry

        auto windowModel = ObjectBroker::model("com.kdab.GammaRay.QuickWindowModel");
        QVERIFY(windowModel);
        ModelTest windowModelTest(windowModel);
        QCOMPARE(windowModel->rowCount(), 1);

        auto itemModel = ObjectBroker::model("com.kdab.GammaRay.QuickItemModel");
        QVERIFY(itemModel);
        ModelTest itemModelTest(itemModel);

        auto sgModel = ObjectBroker::model("com.kdab.GammaRay.QuickSceneGraphModel");
        QVERIFY(sgModel);
        ModelTest sgModelTest(sgModel);

        auto inspector = ObjectBroker::object<QuickInspectorInterface*>();
        QVERIFY(inspector);
        inspector->selectWindow(0);
        QTest::qWait(1);

        view->setSource(QUrl("qrc:/manual/reparenttest.qml"));
        QTest::qWait(1);

        QTest::keyClick(view, Qt::Key_Right);
        QTest::qWait(1);
        QTest::keyClick(view, Qt::Key_Left);
        QTest::qWait(1);
        QTest::keyClick(view, Qt::Key_Right);
        QTest::qWait(1);

        delete view;
        QTest::qWait(1);
    }
};

QTEST_MAIN(QuickInspectorTest)

#include "quickinspectortest.moc"
