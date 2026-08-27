#include <pybind11/embed.h>
#include <pybind11/stl.h>

#include "pydialog.h"

#include <QApplication>
#include <QMessageBox>
#include <QInputDialog>
#include <vector>
#include <string>
#include <iostream>

namespace py = pybind11;

PyDialog pyDialog;

PYBIND11_EMBEDDED_MODULE(pydialog, pw) {
    pw.doc() = "Drishti Paint bridge to Dialog";

    py::class_<PyDialog>(pw, "dialog")
        .def(py::init<>())

        .def("show_message", 
            &PyDialog::showMessage,
            "show_message(title:str, label:str) -> none")

        .def("print_message", 
            &PyDialog::printMessage,
            "print_message(text:str) -> none")

        .def("get_double", 
            &PyDialog::getDouble,
            "get_double(title:str, label:str, value:double, min:double, max:double, decimals:int, step:double) -> double")

        .def("get_int", 
            &PyDialog::getInt,
            "get_double(title:str, label:str, value:int, min:int, max:int, step:int) -> int")

        .def("get_text", 
            &PyDialog::getText,
            "get_text(title:str, label:str, text:str) -> str")

        .def("get_item", 
            &PyDialog::getItem,
            "get_item(title:str, label:str, items:list[str], current:int) -> str");
}

QWidget* PyDialog::m_parent = NULL;
void
PyDialog::setParent(QWidget* parent)
{
  m_parent = parent;
}

QStringList 
PyDialog::toQStringList(const std::vector<std::string>& vec)
{
    QStringList list;
    list.reserve(static_cast<int>(vec.size()));

    for (const auto& s : vec)
        list.append(QString::fromStdString(s));

    return list;
}

void
PyDialog::showMessage(const std::string &title, const std::string &mesg)
{
    QMessageBox::information(m_parent, QString::fromStdString(title), QString::fromStdString(mesg));
}

void
PyDialog::printMessage(const std::string &text)
{
  std::cout << text << "\n";
  qApp->processEvents();
}

int
PyDialog::getInt(const std::string &title,
                 const std::string &label,
                 int value, int min, int max, int step)
{
    bool ok;
    int v = QInputDialog::getInt(m_parent, 
                                QString::fromStdString(title),
                                QString::fromStdString(label),
                                value, min, max, step, &ok);
    if (ok)
        return v;
    else
        return value;
}

double
PyDialog::getDouble(const std::string &title,
                    const std::string &label,
                    double value, double min, double max, int decimals, double step)
{
    bool ok;
    double v = QInputDialog::getDouble(m_parent, 
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
PyDialog::getText(const std::string& title, 
                  const std::string& label,
                  const std::string& text)
{
    bool ok;
    QString t = QInputDialog::getText(m_parent, 
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
PyDialog::getItem(const std::string& title, 
                  const std::string& label,
     const std::vector<std::string>& items,
                               int current)
{
    if (items.size() == 0)
        return "";

    bool ok{};

    QStringList itemsList = PyDialog::toQStringList(items);

    QString item = QInputDialog::getItem(m_parent,
					 QString::fromStdString(title),
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
