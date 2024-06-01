#include <iostream>
#include <string>

#include <boost/python.hpp>
#include <boost/python/module.hpp>

using namespace boost::python;

// Python GIL RAII
class PythonAcquireGil {
public:
	PythonAcquireGil() { _state = PyGILState_Ensure(); };
	virtual ~PythonAcquireGil() { PyGILState_Release(_state); };
private:
	PyGILState_STATE _state;
};

class PythonReleaseGil {
public:
	PythonReleaseGil() { _state = PyEval_SaveThread(); };
	virtual ~PythonReleaseGil() { PyEval_RestoreThread(_state); };
private:
	PyThreadState* _state;
};

// Our interface
class Foo {
public:
	Foo() {}
	void bar(int index, bool opt = true) { std::cout << "bar_int_bool(" << index << ", " << opt << ")" << std::endl << std::endl; }
	void bar(std::string name, bool opt = true) { std::cout << "bar_string_bool(" << name << ", " << opt << ")" << std::endl << std::endl; }
	void bar(int index, int index_2, bool opt = true) { std::cout << "bar_int_int_bool(" << index << ", " << index_2 << ", " << opt << ")" << std::endl << std::endl; }
	void bar(int index, std::string name, bool opt = true) { std::cout << "bar_int_string_bool(" << index << ", " << name << ", " << opt << ")" << std::endl << std::endl; }
	void bar(std::string name, int index, bool opt = true) { std::cout << "bar_string_int_bool(" << name << ", " << index << ", " << opt << ")" << std::endl << std::endl; }
	void bar(std::string name, std::string name_2, bool opt = true) { std::cout << "bar_string_string_bool(" << name << ", " << name_2 << ", " << opt << ")" << std::endl << std::endl; }
};

// Define the py module

// Overload the functions that take 1 parameter and an optional bool
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(foo_bar_overloads_level1, Foo::bar, 1, 2)

// Overload the functions that take 2 parameters and an optional bool
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(foo_bar_overloads_level2, Foo::bar, 2, 3)

BOOST_PYTHON_MODULE(my_module) {

	using namespace boost::python;

	void(Foo:: * bar_int)(int, bool) = &Foo::bar;
	void(Foo:: * bar_string)(std::string, bool) = &Foo::bar;
	void(Foo:: * bar_int_int)(int, int, bool) = &Foo::bar;
	void(Foo:: * bar_int_string)(int, std::string, bool) = &Foo::bar;
	void(Foo:: * bar_string_int)(std::string, int, bool) = &Foo::bar;
	void(Foo:: * bar_string_string)(std::string, std::string, bool) = &Foo::bar;

	class_<Foo>("Foo")
		.def("bar", bar_int, foo_bar_overloads_level1(args("index", "opt")))
		.def("bar", bar_string, foo_bar_overloads_level1(args("name", "opt")))
		.def("bar", bar_int_int, foo_bar_overloads_level2(args("index", "index2", "opt")))
		.def("bar", bar_int_string, foo_bar_overloads_level2(args("index", "string", "opt")))
		.def("bar", bar_string_int, foo_bar_overloads_level2(args("name", "index", "opt")))
		.def("bar", bar_string_string, foo_bar_overloads_level2(args("name", "name_2", "opt")));
}

void print_error()
{
	PythonAcquireGil lock;

	PyObject* ptype, * pvalue, * ptraceback;
	PyErr_Fetch(&ptype, &pvalue, &ptraceback);

	try {
		//Extract error message
		std::string error_message = extract<std::string>(pvalue);
		std::cout << error_message << std::endl;
	} catch (error_already_set&) {
		PyObject* ptype, * pvalue, * ptraceback;
		PyErr_Fetch(&ptype, &pvalue, &ptraceback);

		//Extract error message
		std::string error_message = extract<std::string>(pvalue);
		std::cout << error_message << std::endl;
	}
}

int main() {

	try {

		PyImport_AppendInittab("my_module", &PyInit_my_module);

		Py_Initialize();

		object main_module = import("__main__");

		object main_namespace = main_module.attr("__dict__");

		object pyModule((handle<>(PyImport_ImportModule("my_module"))));

		object pyNamespace = pyModule.attr("__dict__");

		// Run some python code to test the function calls.
		str pyCode(R"(

import my_module

foo = my_module.Foo()

print("bar_int_bool_default")
foo.bar(1)

print("bar_int_bool_nodefault")
foo.bar(1, False)

print("bar_string_bool_default")
foo.bar("a")

print("bar_string_bool_nodefault")
foo.bar("a", False)

print("bar_int_int_bool_default")
foo.bar(1, 2)

print("bar_int_int_bool_nodefault")
foo.bar(1, 2, False)

print("bar_int_string_bool_default")
foo.bar(1, "a")

print("bar_int_string_bool_nodefault")
foo.bar(1, "a", False)

print("bar_string_int_bool_default")
foo.bar("a", 1)

print("bar_string_int_bool_nodefault")
foo.bar("a", 1, False)

print("bar_string_string_bool_default")
foo.bar("a", "b")

print("bar_string_string_bool_nodefault")
foo.bar("a", "b", False)

)");

		object obj = exec(pyCode, main_namespace);
	}
	catch (error_already_set&)
	{
		print_error();
	}

	Py_Finalize();

	return 0;
}