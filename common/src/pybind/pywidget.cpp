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

PyWidget pyWidget;

PYBIND11_EMBEDDED_MODULE(pywidget, pw) {
    pw.doc() = "Drishti Paint bridge to Widgets";

    py::class_<PyWidget>(pw, "widget")
        .def(py::init<>())

        .def("show_message", 
            &PyWidget::showMessage,
            "show_message(title:str, label:str) -> none")

        .def("get_double", 
            &PyWidget::getDouble,
            "get_double(title:str, label:str, value:double, min:double, max:double, decimals:int, step:double) -> double")

        .def("get_int", 
            &PyWidget::getInt,
            "get_double(title:str, label:str, value:int, min:int, max:int, step:int) -> int")

        .def("get_text", 
            &PyWidget::getText,
            "get_text(title:str, label:str, text:str) -> str")

        .def("get_item", 
            &PyWidget::getItem,
            "get_item(title:str, label:str, items:list[str], current:int) -> str");
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

int
PyWidget::getInt(const std::string &title,
                 const std::string &label,
                 int value, int min, int max, int step)
{
    bool ok;
    int v = QInputDialog::getInt(0, 
                                QString::fromStdString(title),
                                QString::fromStdString(label),
                                value, min, max, step, &ok);
    if (ok)
        return v;
    else
        return value;
}

double
PyWidget::getDouble(const std::string &title,
                    const std::string &label,
                    double value, double min, double max, int decimals, double step)
{
    bool ok;
    double v = QInputDialog::getDouble(0, 
                                        QString::fromStdString(title),
                                        QString::fromStdString(label),
                                        value, min, max, decimals, 
                                        &ok, Qt::WindowFlags(), step);
    if (ok)
        return v;
    else
        return value;
}

std::string
PyWidget::getText(const std::string& title, 
                  const std::string& label,
                  const std::string& text)
{
    bool ok;
    QString t = QInputDialog::getText(0, 
                                      QString::fromStdString(title),
                                      QString::fromStdString(label), 
                                      QLineEdit::Normal,
                                      QString::fromStdString(text),
                                      &ok);
    if (ok)
        return t.toStdString();
    else
        return text;
}

std::string
PyWidget::getItem(const std::string& title, 
                  const std::string& label,
     const std::vector<std::string>& items,
                               int current)
{
    if (items.size() == 0)
        return "";

    bool ok{};

    QStringList itemsList = PyWidget::toQStringList(items);

    QString item = QInputDialog::getItem(0, QString::fromStdString(title),
                                            QString::fromStdString(label), 
                                            itemsList, current, false, &ok);
    if (ok && !item.isEmpty())
        return item.toStdString();
    else
        {
            current = qBound(0, current, (int)items.size());
            return items[current];
        }
}