#ifndef LABELBUDDY_COMPAT_H
#define LABELBUDDY_COMPAT_H

#include <QFontMetrics>
#include <QString>

namespace labelbuddy {

/// use QFontMetrics::width before Qt 5.11 and horizontalAdvance after
int text_width(const QFontMetrics& metrics, const QString& text);

} // namespace labelbuddy

#endif
