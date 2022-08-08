#ifndef LABELBUDDY_TEST_DOC_LIST_MODEL_H
#define LABELBUDDY_TEST_DOC_LIST_MODEL_H

#include <QTest>

namespace labelbuddy {
class TestDocListModel : public QObject {
  Q_OBJECT
private slots:
  void testDeleteDocs();
  void testFilters();
  void testUpdatingResults();
};
} // namespace labelbuddy
#endif
