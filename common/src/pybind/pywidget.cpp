#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <pybind11/complex.h>
#include <pybind11/functional.h>
#include <pybind11/chrono.h>

#include "pywidget.h"

#include <QMessageBox>
#include <QInputDialog>
#include <vector>
#include <string>

namespace py = pybind11;


PYBIND11_EMBEDDED_MODULE(pywidget, pw) {
    pw.doc() = "Drishti Paint bridge to Widgets";

    py::class_<PyWidget>(pw, "widget")
        .def(py::init<>())
        .def("show_message", &PyWidget::showMessage)
        .def("get_item", &PyWidget::getItem);
}


QStringList 
PyWidget::toQStringList(const std::vector<std::string>& vec)
{
    QStringList list;
    list.reserve(static_cast<int>(vec.size()));

    for (const auto& s : vec)
        list.append(QString::fromStdString(s));

    return list;
}

void
PyWidget::showMessage(const std::string &title, const std::string &mesg)
{
    QMessageBox::information(0, QString::fromStdString(title), QString::fromStdString(mesg));
}

std::string
PyWidget::getItem(const std::string& title, 
                  const std::string& label,
     const std::vector<std::string>& items,
                               int current)
{
bool ok{};

QStringList itemsList = PyWidget::toQStringList(items);

QString item = QInputDialog::getItem(0, QString::fromStdString(title),
                                        QString::fromStdString(label), 
                                        itemsList, current, false, &ok);
if (ok && !item.isEmpty())
    return item.toStdString();
else
    return "";
}