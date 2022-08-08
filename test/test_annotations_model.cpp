#include <QCryptographicHash>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "annotations_model.h"
#include "database.h"
#include "test_annotations_model.h"
#include "testing_utils.h"

namespace labelbuddy {
void TestAnnotationsModel::testAddAndDeleteAnnotations() {
  QTemporaryDir tmpDir{};
  auto dbName = prepareDb(tmpDir);
  AnnotationsModel model{};
  model.setDatabase(dbName);
  int id{};
  id = model.addAnnotation(2, 3, 5);
  QCOMPARE(id, 1);
  id = model.addAnnotation(1, 7, 9);
  QCOMPARE(id, 2);
  id = model.addAnnotation(10, 7, 9);
  QCOMPARE(id, -1);
  id = model.addAnnotation(3, 10, 12);
  QCOMPARE(id, 3);
  model.deleteAnnotation(1);
  QSqlQuery query(QSqlDatabase::database(dbName));
  query.exec(
      "select label_id from annotation where doc_id = 1 order by label_id;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 1);
  auto labelsInfo = model.getLabelsInfo();
  QCOMPARE(labelsInfo[3].color, QString("#98df8a"));
  auto annotations = model.getAnnotationsInfo();
  QCOMPARE(annotations[3].endChar, 12);
}

void TestAnnotationsModel::testNavigation() {
  QTemporaryDir tmpDir{};
  auto dbName = prepareDb(tmpDir);
  AnnotationsModel model{};
  model.setDatabase(dbName);
  model.visitNext();
  QCOMPARE(model.currentDocPosition(), 1);
  model.addAnnotation(2, 3, 5);
  model.visitNextUnlabelled();
  QCOMPARE(model.currentDocPosition(), 2);
  QVERIFY(model.getContent().startsWith("document 2"));
  model.visitNext();
  model.visitNextUnlabelled();
  QCOMPARE(model.currentDocPosition(), 4);
  for (int i = 0; i != 20; ++i) {
    model.visitNextUnlabelled();
  }
  QCOMPARE(model.currentDocPosition(), 5);
  QCOMPARE(model.getAnnotationsInfo().size(), 0);
  model.visitPrevLabelled();
  QCOMPARE(model.currentDocPosition(), 1);
  QCOMPARE(model.getAnnotationsInfo().size(), 1);
  model.visitPrevLabelled();
  model.visitPrevLabelled();
  QCOMPARE(model.currentDocPosition(), 1);
  model.visitPrev();
  QCOMPARE(model.currentDocPosition(), 0);
  model.visitNextUnlabelled();
  QCOMPARE(model.currentDocPosition(), 2);
  model.visitPrev();
  QSqlQuery query(QSqlDatabase::database(dbName));
  query.exec("delete from document where id = 2;");
  query.next();
  model.checkCurrentDoc();
  QCOMPARE(model.currentDocPosition(), 0);
  query.exec("delete from document;");
  query.next();
  model.checkCurrentDoc();
  QCOMPARE(model.getContent(), QString(""));
}

void TestAnnotationsModel::testSurrogatePairs() {
  QTemporaryDir tmpDir{};
  DatabaseCatalog catalog{};
  catalog.openDatabase(tmpDir.filePath("db"));
  QSqlQuery query(QSqlDatabase::database(catalog.getCurrentDatabase()));
  query.exec("insert into label(name) values ('label');");
  query.prepare("insert into document(content, content_md5) values (?, ?);");
  QString content("𝄞𝄞-𝄞");
  query.bindValue(0, content);
  auto hash =
      QCryptographicHash::hash(content.toUtf8(), QCryptographicHash::Md5);
  query.bindValue(1, hash);
  query.exec();
  query.exec("select count(*) from document;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 1);
  AnnotationsModel model{};
  model.setDatabase(catalog.getCurrentDatabase());

  QCOMPARE(model.codePointIdxToUtf16Idx(0), 0);
  QCOMPARE(model.codePointIdxToUtf16Idx(1), 2);
  QCOMPARE(model.codePointIdxToUtf16Idx(2), 4);
  QCOMPARE(model.codePointIdxToUtf16Idx(3), 5);
  QCOMPARE(model.codePointIdxToUtf16Idx(4), 7);

  QCOMPARE(model.utf16IdxToCodePointIdx(0), 0);
  QCOMPARE(model.utf16IdxToCodePointIdx(1), 0);
  QCOMPARE(model.utf16IdxToCodePointIdx(2), 1);
  QCOMPARE(model.utf16IdxToCodePointIdx(3), 1);
  QCOMPARE(model.utf16IdxToCodePointIdx(4), 2);
  QCOMPARE(model.utf16IdxToCodePointIdx(5), 3);
  QCOMPARE(model.utf16IdxToCodePointIdx(6), 3);

  model.addAnnotation(1, 0, 2);
  query.exec("select start_char, end_char from annotation where rowid = 1;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 0);
  QCOMPARE(query.value(1).toInt(), 1);

  model.addAnnotation(1, 4, 7);
  query.exec("select start_char, end_char from annotation where rowid = 2;");
  query.next();
  QCOMPARE(query.value(0).toInt(), 2);
  QCOMPARE(query.value(1).toInt(), 4);

  content = "éé";
  hash = QCryptographicHash::hash(content.toUtf8(), QCryptographicHash::Md5);
  query.prepare("insert into document(content, content_md5) values (?, ?);");
  query.bindValue(0, content);
  query.bindValue(1, hash);
  query.exec();
  model.visitDoc(2);
  QCOMPARE(model.getContent(), content);

  QCOMPARE(model.codePointIdxToUtf16Idx(0), 0);
  QCOMPARE(model.codePointIdxToUtf16Idx(1), 1);
  QCOMPARE(model.codePointIdxToUtf16Idx(2), 2);

  QCOMPARE(model.utf16IdxToCodePointIdx(0), 0);
  QCOMPARE(model.utf16IdxToCodePointIdx(1), 1);
  QCOMPARE(model.utf16IdxToCodePointIdx(2), 2);
}

} // namespace labelbuddy
