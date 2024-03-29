#include <QColor>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QVariant>

#include "annotations_list_model.h"
#include "annotations_model.h"
#include "user_roles.h"

#include "test_annotations_list_model.h"
#include "testing_utils.h"

namespace labelbuddy {

void TestAnnotationsListModel::testAddAndDeleteAnnotations() {
  QTemporaryDir tmpDir{};
  auto dbName = prepareDb(tmpDir);
  AnnotationsModel model{};
  model.setDatabase(dbName);
  AnnotationsListModel listModel{};
  listModel.setSourceModel(&model);

  model.visitFirstDoc();
  model.addAnnotation(2, 3, 5);
  model.addAnnotation(1, 0, 2);
  QCOMPARE(listModel.rowCount(), 2);
  auto labelName = listModel.data(listModel.index(0, 0), Roles::LabelNameRole)
                       .value<QString>();
  // the label name correspondig to id 2 (second label in test_labels.json)
  QCOMPARE(labelName, QString("label: Resumption of the session"));
  // delete annotation by id, ie delete the first one
  model.deleteAnnotation(1);
  QCOMPARE(listModel.rowCount(), 1);
  labelName = listModel.data(listModel.index(0, 0), Roles::LabelNameRole)
                  .value<QString>();
  // the second annotation, created with label id 1, remains
  QCOMPARE(labelName, QString("label: Reinício da sessão"));
  model.updateAnnotationExtraData(2, "hello");
  auto extra =
      listModel.data(listModel.index(0, 0), Roles::AnnotationExtraDataRole)
          .value<QString>();
  QCOMPARE(extra, "hello");
}

void TestAnnotationsListModel::testRoles() {
  QTemporaryDir tmpDir{};
  auto dbName = prepareDb(tmpDir);
  QSqlQuery query(QSqlDatabase::database(dbName));
  // add a document with a character out of the BMP
  query.exec("insert into document(content, content_md5) values "
             "('0🙂3456789abcdef', '427e2382a5a167db4421dec0bcb8aeca');");
  query.next();

  AnnotationsModel model{};
  model.setDatabase(dbName);
  AnnotationsListModel listModel{};
  listModel.setSourceModel(&model);
  model.visitDoc(7);
  // the annotation falls 12 characters after the first surrogate char of "🙂"
  // -- this way the default prefix length would cut the character in half, to
  // check that the length is correctly adjusted.
  model.addAnnotation(2, 14, 15);

  auto badIndex = listModel.data(listModel.index(1000, 0), Qt::BackgroundRole);
  QCOMPARE(badIndex, QVariant{});

  auto prefix =
      listModel.data(listModel.index(0, 0), Roles::AnnotationPrefixRole)
          .value<QString>();
  QCOMPARE(prefix, "🙂3456789abcd");

  auto selection =
      listModel.data(listModel.index(0, 0), Roles::SelectedTextRole)
          .value<QString>();
  QCOMPARE(selection, "e");

  auto suffix =
      listModel.data(listModel.index(0, 0), Roles::AnnotationSuffixRole)
          .value<QString>();
  QCOMPARE(suffix, "f");

  auto color = listModel.data(listModel.index(0, 0), Qt::BackgroundRole)
                   .value<QColor>()
                   .name();
  QCOMPARE(color, "#ffbb78");

  auto id = listModel.data(listModel.index(0, 0), Roles::AnnotationIdRole)
                .value<int>();
  QCOMPARE(id, 1);

  auto startChar =
      listModel.data(listModel.index(0, 0), Roles::AnnotationStartCharRole)
          .value<int>();
  QCOMPARE(startChar, 14);

  auto idx = listModel.indexForAnnotationId(1);
  QCOMPARE(idx.row(), 0);
  idx = listModel.indexForAnnotationId(10);
  QCOMPARE(idx.isValid(), false);

  query.exec("insert into document( content, content_md5) values "
             "('"
             "00000000000123444444444444444444444444444444444444444444444444444"
             "44444444444444444444444444444444444444444444444444444444444444444"
             "44444444444444444444444444444444444444444444444444444444444444444"
             "44444𝄞5', '3c29867197ccceb811ae72e6fbc2d9a4')");
  query.next();

  model.visitDoc(8);
  model.addAnnotation(2, 12, 13);
  model.addAnnotation(2, 13, 14);
  model.addAnnotation(2, 14, 15);
  model.addAnnotation(2, 15, 16);
  model.addAnnotation(2, 2, 200);
  model.addAnnotation(2, 13, 203);

  suffix = listModel.data(listModel.index(0, 0), Roles::AnnotationSuffixRole)
               .value<QString>();
  QVERIFY(suffix.endsWith("4"));

  suffix = listModel.data(listModel.index(1, 0), Roles::AnnotationSuffixRole)
               .value<QString>();
  QVERIFY(suffix.endsWith("𝄞"));

  suffix = listModel.data(listModel.index(2, 0), Roles::AnnotationSuffixRole)
               .value<QString>();
  QVERIFY(suffix.endsWith("𝄞"));

  suffix = listModel.data(listModel.index(3, 0), Roles::AnnotationSuffixRole)
               .value<QString>();
  QVERIFY(suffix.endsWith("5"));

  suffix = listModel.data(listModel.index(4, 0), Roles::AnnotationSuffixRole)
               .value<QString>();
  QCOMPARE(suffix, "");

  selection = listModel.data(listModel.index(5, 0), Roles::SelectedTextRole)
                  .value<QString>();
  QVERIFY(selection.endsWith("𝄞"));
}

} // namespace labelbuddy
