#include <QtGlobal>

#include "compat.h"

namespace labelbuddy {

int textWidth(const QFontMetrics& metrics, const QString& text) {
#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
  return metrics.width(text);
#else
  return metrics.horizontalAdvance(text);
#endif
}
} // namespace labelbuddy
