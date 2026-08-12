QT -= gui
QT += core

CONFIG += console c++11
CONFIG -= app_bundle

TEMPLATE = app
TARGET = raw_file_safety_smoke

SOURCES = raw_file_safety_smoke.cpp
HEADERS = ../plugins/rawfileutils.h
