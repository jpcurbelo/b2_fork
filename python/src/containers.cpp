//This file is part of Bertini 2.
//
//python/src/containers.cpp is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.
//
//python/src/containers.cpp is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with python/src/containers.cpp.  If not, see <http://www.gnu.org/licenses/>.
//
// Copyright(C) 2017-2018 by Bertini2 Development Team
//
// See <http://www.gnu.org/licenses/> for a copy of the license, 
// as well as COPYING.  Bertini2 is provided with permitted 
// additional terms in the b2/licenses/ directory.

// individual authors of this file include:
//
//  James Collins
//  West Texas A&M University
//  Spring 2016
//
//  silviana amethyst
//  UWEC
//  Spring 2018
//
//
//  python/src/containers.cpp:  source file for exposing trackers to python.


#include "containers_export.hpp"

namespace bertini{
	namespace python{

template<typename T>
template<typename PyClass>
void ListVisitor<T>::visit(PyClass& cl) const
{
	cl

	.def(vector_indexing_suite< T , true >())
	// By default indexed elements are returned by proxy. This can be
    // disabled by supplying *true* in the NoProxy template parameter.

	.def("__str__", &ListVisitor::__str__)
	.def("__repr__", &ListVisitor::__repr__)
	;
}


void ExportContainers()
{
	scope current_scope;
	std::string new_submodule_name(extract<const char*>(current_scope.attr("__name__")));
	new_submodule_name.append(".list");
	object new_submodule(borrowed(PyImport_AddModule(new_submodule_name.c_str())));
	current_scope.attr("list") = new_submodule;

	scope new_submodule_scope = new_submodule;
	new_submodule_scope.attr("__doc__") = "List types for PyBertini";
 

	boost::python::converter::registry::push_back(&pylist_converter<bertini::VariableGroup>::convertible
	    , &pylist_converter<bertini::VariableGroup>::construct
	    , boost::python::type_id<bertini::VariableGroup>());



	// std::vector of Rational Node ptrs
	using T1 = std::vector<std::shared_ptr< bertini::node::Rational > >;
	class_< T1 >("Rational")
	.def(ListVisitor<T1>())
	;

	// The VariableGroup vector container
	using T2 = bertini::VariableGroup;
	class_< T2 >("VariableGroup")
	.def(ListVisitor<T2>())
	.def("__init__", boost::python::make_constructor(&create_MyClass<T2>))
	;
	
	// std::vector of ints
	using T3 = std::vector<int>;
	class_< T3 >("int")
	.def(ListVisitor<T3>())
	;


	// std::vector of VariableGroups
	using T4 = std::vector<bertini::VariableGroup>;
	class_< T4 >("OfVariableGroup")
	.def(ListVisitor<T4>())
	;


	// std::vector of Function Node ptrs
	using T5 = std::vector<std::shared_ptr< bertini::node::Function > >;
	class_< T5 >("Function")
	.def(ListVisitor<T5>())
	;


	// std::vector of Jacobian Node ptrs
	using T6 = std::vector<std::shared_ptr< bertini::node::Jacobian > >;
	class_< T6 >("Jacobian")
	.def(ListVisitor<T6>())
	;




}; // export containers

	}
}