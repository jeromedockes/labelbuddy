#ifndef LABELBUDDY_UTILS_H
#define LABELBUDDY_UTILS_H

#include <QModelIndexList>
#include <QString>
#include <QUrl>

namespace labelbuddy {

QString get_version();

QUrl get_doc_url();

QModelIndexList::const_iterator
find_first_in_col_0(const QModelIndexList& indices);

} // namespace labelbuddy
#endif
