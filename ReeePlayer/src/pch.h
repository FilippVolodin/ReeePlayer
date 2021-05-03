#ifndef PCH_H
#define PCH_H

#include <windows.h>

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
#include <QtXml/QDomNode>
#include <QHash>
#include <QAbstractItemModel>
#include <QtXml/QDomDocument>
#include <QModelIndex>
#include <QPainter>
#include <QButtonGroup>
#include <QTextDocument>
#include <QJsonObject>
#include <QStylePainter>
#include <QSpinBox>
#include <QLineEdit>

//#include <QSetting>

__pragma(warning(push))
__pragma(warning(disable:4127))
#include <QtSql>
__pragma(warning(pop))

#include <algorithm>
#include <optional>
#include <set>
#include <stack>

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#include <vlc/vlc.h>

//using  int64_t;

#endif //PCH_H