#ifndef PCH_H
#define PCH_H

// Prevent #define max (and min)
#define NOMINMAX
#include <windows.h>

#include <QtWidgets/QApplication>
#include <QtGlobal>
#include <QWidget>
#include <QToolBar>
#include <QPlainTextEdit>
#include <QTreeWidget>
#include <QMenu>
#include <QDropEvent>
#include <QTimer>
#include <QMainWindow>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QTime>
#include <QShortcut>
#include <QProxyStyle>
#include <QMessageBox>
#include <QFileDialog>
#include <QSet>
#include <QtMath>
#include <QHash>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QPainter>
#include <QButtonGroup>
#include <QTextDocument>
#include <QJsonObject>
#include <QStylePainter>
#include <QSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonArray>
#include <QMimeData>
#include <QStandardPaths>

#include <algorithm>
#include <optional>
#include <set>
#include <stack>
#include <thread>

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#include <vlc/vlc.h>

#endif //PCH_H