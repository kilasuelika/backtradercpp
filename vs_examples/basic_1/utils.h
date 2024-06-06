// utils.h
#ifndef UTILS_H
#define UTILS_H

#include <DataFrame/DataFrame.h>
#include <numpy/arrayobject.h>
#include <Python.h>

using namespace hmdf;
using ULDataFrame = StdDataFrame<unsigned long>;

ULDataFrame convert_to_ULDataFrame(PyObject* numpy_array);

#endif // UTILS_H