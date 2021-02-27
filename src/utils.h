#ifndef LABELBUDDY_UTILS_H
#define LABELBUDDY_UTILS_H

#include <QCommandLineParser>
#include <QModelIndexList>
#include <QRegularExpression>
#include <QString>
#include <QUrl>

namespace labelbuddy {

QString get_version();

QUrl get_doc_url();

void prepare_parser(QCommandLineParser& parser);

QModelIndexList::const_iterator
find_first_in_col_0(const QModelIndexList& indices);

QRegularExpression shortcut_key_pattern(bool accept_empty = false);

} // namespace labelbuddy
#endif
