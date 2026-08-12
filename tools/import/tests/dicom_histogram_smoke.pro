QT -= gui
QT -= core

CONFIG += console c++11
CONFIG -= app_bundle

TEMPLATE = app
TARGET = dicom_histogram_smoke

SOURCES = dicom_histogram_smoke.cpp
HEADERS = ../plugins/dicom/dicomhistogramutils.h
