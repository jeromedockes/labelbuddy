#ifndef LABELBUDDY_TEST_MAIN_WINDOW_H
#define LABELBUDDY_TEST_MAIN_WINDOW_H

#include <QTest>

namespace labelbuddy {

class TestLabelBuddy : public QObject {
  Q_OBJECT

private slots:

  void testLabelBuddy();
};

} // namespace labelbuddy
#endif
