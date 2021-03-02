#ifndef LABELBUDDY_UTILS_H
#define LABELBUDDY_UTILS_H

#include <QCommandLineParser>
#include <QModelIndexList>
#include <QRegularExpression>
#include <QString>
#include <QUrl>

/// \file
/// Misc. utilities

namespace labelbuddy {

/// The labelbuddy version eg '0.0.3'
QString get_version();

/// URL to the html documentation

/// Searched in /usr/share/doc, the parent directory of the running program, and
/// if not found, return URL to the online doc.
QUrl get_doc_url();

void prepare_parser(QCommandLineParser& parser);

/// Find the first index that is in the column 0

/// If none of the indices are in column 0 return `indices.constEnd()`.
/// LabelList and DocList display column 0 but also have a hidden column
/// containing the row id. This is used to find the first selected item that is
/// in the displayed column -- hidden items could be selected by clicking
/// "select all".
///
/// In practice this is a redundant check because the corresponding models'
/// `flags` make the other columns non-selectable.
QModelIndexList::const_iterator
find_first_in_col_0(const QModelIndexList& indices);

/// ATM single lower-case letters. If `accept_empty` the empty string also
/// allowed.

/// Empty string can be acceptable to clear the label's shortcut in Dataset tab.
QRegularExpression shortcut_key_pattern(bool accept_empty = false);

} // namespace labelbuddy
#endif
