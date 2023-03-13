#include <Qt>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <qnamespace.h>

#include "annotations_list.h"
#include "annotations_model.h"

#include "test_annotations_list.h"
#include "testing_utils.h"


namespace labelbuddy {

  void TestAnnotationsList::testAnnotationsList() {
    QTemporaryDir tmpDir{};
    auto dbName = prepareDb(tmpDir);
    AnnotationsModel model{};
    model.setDatabase(dbName);
    model.visitFirstDoc();

    AnnotationsList annoList{};
    auto lv = annoList.findChild<QListView*>();

    annoList.setModel(&model);

    annoList.show();

    model.addAnnotation(1, 10, 12);
    QTest::qWait(500);
    model.addAnnotation(2, 3, 15);
    QTest::qWait(500);

    QCOMPARE(lv->model()->rowCount(), 2);

    annoList.selectAnnotation(2);
    QTest::qWait(500);
    // annotations are sorted by startChar (as numbers)
    QCOMPARE(lv->selectionModel()->selectedIndexes()[0].row(), 0);

    // reset annotations deselects all
    annoList.resetAnnotations();
    QCOMPARE(lv->selectionModel()->selectedIndexes().size(), 0);

    // as does selecting an invalid annotation (id = -1)
    annoList.selectAnnotation(2);
    annoList.selectAnnotation(-1);
    QCOMPARE(lv->selectionModel()->selectedIndexes().size(), 0);

    // changing document resets the annotations
    annoList.selectAnnotation(2);
    model.visitNext();
    QCOMPARE(lv->model()->rowCount(), 0);
    QCOMPARE(lv->selectionModel()->selectedIndexes().size(), 0);

    QSignalSpy spy(&annoList, SIGNAL(clicked()));
    QTest::mouseClick(lv->viewport(), Qt::LeftButton);
    QCOMPARE(spy.count(), 1);

    annoList.resetAnnotations();
  }
}
