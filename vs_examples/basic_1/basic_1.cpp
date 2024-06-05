#include <iostream>
#include "../../include/backtradercpp/Cerebro.hpp"
#include <Python.h>
#include <numpy/arrayobject.h>
#include "../../include/backtradercpp/DataFeeds.hpp" // 包含 ULDataFrame 的头文件
#include "DataFrame/DataFrame.h"

using namespace backtradercpp;
using namespace std;
using namespace hmdf;


struct SimpleStrategy : strategy::GenericStrategy {
    void run() override {
        // Buy assets at 6th day. Index starts from 0, so index 5 means 6th day.
        if (time_index() == 5) {
            for (int j = 0; j < data(0).assets(); ++j) {
                if (data(0).valid(-1, j)) {
                    // Buy 10 asset j at the price of latest day(-1) on the broker 0.
                    buy(0, j, data(0).open(-1, j), 10);
                }
            }
        }
    }
};
int main() {
    Cerebro cerebro;
    // non_delimit_date is a function that convert date string like "20200101" to standard format.
    //  0.0005 and 0.001 are commission rate for long and short trading.
    cerebro.add_broker(
        broker::BaseBroker(0.0005, 0.001)
            .set_feed(feeds::CSVTabPriceData("../../example_data/CSVTabular/djia.csv",
                                             feeds::TimeStrConv::non_delimited_date)));
    cerebro.add_strategy(std::make_shared<SimpleStrategy>());
    cerebro.run();
}

using namespace hmdf;


ULDataFrame convert_to_ULDataFrame(PyObject* numpy_array) {
    if (!PyArray_Check(numpy_array)) {
        throw std::invalid_argument("Expected a NumPy array");
    }

    PyArrayObject* array = reinterpret_cast<PyArrayObject*>(numpy_array);
    if (PyArray_NDIM(array) != 2) {
        throw std::invalid_argument("Expected a 2-dimensional NumPy array");
    }

    if (PyArray_TYPE(array) != NPY_DOUBLE) {
        throw std::invalid_argument("Expected a NumPy array with double data type");
    }

    npy_intp* shape = PyArray_SHAPE(array);
    size_t rows = shape[0];
    size_t cols = shape[1];

    ULDataFrame df;

    for (size_t j = 0; j < cols; ++j) {
        std::vector<double> column_data(rows);
        for (size_t i = 0; i < rows; ++i) {
            column_data[i] = *reinterpret_cast<double*>(PyArray_GETPTR2(array, i, j));
        }
        std::string column_name = "Column_" + std::to_string(j);
        df.load_column<double>(column_name, column_data);
    }

    return df;
}

extern "C" {
    __declspec(dllexport) void runBacktrader(PyObject* numpy_array) {
        if (!Py_IsInitialized()) {
            Py_Initialize();
        }

        try {
            import_array();

            ULDataFrame df = convert_to_ULDataFrame(numpy_array);
            backtradercpp::feeds::DataFrameTabPriceData priceData(df);

            Cerebro cerebro;
            cerebro.add_broker(
                broker::BaseBroker(100000, 0.0005, 0.001)
                    .set_df_feed(priceData)
            );
            cerebro.add_strategy(std::make_shared<SimpleStrategy>());
            cerebro.run();
        } catch (const std::exception &e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
}


// 用于初始化Python解释器
void initialize_python() {
    Py_Initialize();
    if (!Py_IsInitialized()) {
        std::cerr << "Python initialization failed!" << std::endl;
        exit(1);
    }
}

// 用于关闭Python解释器
void finalize_python() {
    if (Py_IsInitialized()) {
        Py_Finalize();
    }
}

// 用于读取和处理Python的pandas DataFrame
void process_dataframe(PyObject* dataframe) {
    // 确保传入的是一个pandas DataFrame对象
    if (!PyObject_HasAttrString(dataframe, "to_numpy")) {
        std::cerr << "Object is not a pandas DataFrame!" << std::endl;
        return;
    }

    // 调用DataFrame的to_numpy方法将其转换为NumPy数组
    PyObject* numpy_array = PyObject_CallMethod(dataframe, "to_numpy", nullptr);
    if (!numpy_array) {
        std::cerr << "Failed to convert DataFrame to NumPy array!" << std::endl;
        return;
    }

    // 获取NumPy数组的形状
    PyObject* shape = PyObject_GetAttrString(numpy_array, "shape");
    if (!shape) {
        std::cerr << "Failed to get shape of NumPy array!" << std::endl;
        Py_DECREF(numpy_array);
        return;
    }

    // 打印NumPy数组的形状
    Py_ssize_t rows = PyLong_AsSsize_t(PyTuple_GetItem(shape, 0));
    Py_ssize_t cols = PyLong_AsSsize_t(PyTuple_GetItem(shape, 1));
    std::cout << "NumPy array shape: " << rows << " rows, " << cols << " cols" << std::endl;

    // 进一步处理NumPy数组，如提取数据进行计算等
    // ...

    Py_DECREF(shape);
    Py_DECREF(numpy_array);
}
