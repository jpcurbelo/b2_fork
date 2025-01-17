
//This file is part of Bertini 2.
//
//straight_line_program.hpp is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.
//
//straight_line_program.hpp is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with straight_line_program.hpp.  If not, see <http://www.gnu.org/licenses/>.
//
// Copyright(C) 2021 by Bertini2 Development Team
//
// See <http://www.gnu.org/licenses/> for a copy of the license,
// as well as COPYING.  Bertini2 is provided with permitted
// additional terms in the b2/licenses/ directory.

// individual authors of this file include:
// silviana amethyst, university of wisconsin eau claire
// michael mumm, university of wisconsin eau claire

/**
\file straight_line_program.hpp

\brief Provides the bertini::StraightLineProgram class.
*/

#ifndef BERTINI_SLP_HPP
#define BERTINI_SLP_HPP

#pragma once

#include <assert.h>
#include <vector>
#include <map>

#include "bertini2/mpfr_complex.hpp"
#include "bertini2/mpfr_extensions.hpp"
#include "bertini2/eigen_extensions.hpp"
#include "bertini2/function_tree/forward_declares.hpp"
#include "bertini2/detail/visitor.hpp"

// code copied from Bertini1's file include/bertini.h


/*

typedef struct
{
	int num_funcs;
	int num_hom_var_gp;
	int num_var_gp;
	int *type; // 0 - hom_var_gp, 1 - var_gp
	int *size; // size of the group of the user listed variables (total size = size + type)
} preproc_data;


typedef struct
{
	point_d funcVals;
	point_d parVals;
	vec_d parDer;
	mat_d Jv;
	mat_d Jp;
} eval_struct_d;

typedef struct
{
	point_mp funcVals;
	point_mp parVals;
	vec_mp parDer;
	mat_mp Jv;
	mat_mp Jp;
} eval_struct_mp;


The straight-line program structure.  This is the way that polynomials are stored internally.
typedef struct {
	int *prog;     //  The program instructions. (a big integer array)
	int  size;     //  size of the instruction program.
	int  memSize;  // Amount of memory it needs in workspace (for temp and final results).
	num_t *nums;   // The array of real numbers.
	int precision; // The precision at which evaluation should occur

	// INFO NEEDED FOR M-HOM:
	int num_var_gps;  // The total number of variable groups (i.e., m from m-hom).
	int *var_gp_sizes;  // The size of each of the groups.
	int index_of_first_number_for_proj_trans;  // The address of the first number used in the projective transformation polynomials.

	// STOP LOCATIONS:
	int  numInstAtEndUpdate; // instruction number at end of update. i.e. i = 0; while (i < numInstAtEndUpdate) ..
	int  numInstAtEndParams; // instruction number at end of params. i.e. i = numInstAtEndUpdate; while (i < numInstAtEndParams) ..
	int  numInstAtEndFnEval; // instruction number at end of function eval. i.e. i = numInstAtEndParams; while (i < numInstAtEndFnEval) ..
	int  numInstAtEndPDeriv; // instruction number at end of param diff. i.e. i = numInstAtEndFnEval; while (i < numInstAtEndPDeriv) ..
	int  numInstAtEndJvEval; // instruction number at end of Jv eval. i.e. i = numInstAtEndPDeriv; while (i < numInstAtEndJvEval) ..
													 // for Jp eval: i = numInstAtEndJvEval; while (i < size) ..

	// INPUT AMOUNTS:
	int  numVars;  //  Number of variables in the function being computed.
	int  numPathVars;  //  Number of path variables.  Ought to be 1 usually.
	int  numNums;  //  Number of real numbers used in evaluation.
	int  numConsts;  //  Number of constants.

	// OUTPUT AMOUNTS:
	int  numPars;  //  Number of parameters
	int  numFuncs; //  Number of coordinate functions in the homotopy.
	int  numSubfuncs;  //  Number of subfunctions.

	// INPUT LOCATIONS:
	int  inpVars;  //  Where the input variable values are stored.
	int  inpPathVars;  //  Where the values of the path variables are stored.
	int  IAddr;  //  Where the constant I is stored.
	int  numAddr;  //  Where the first num_t is stored.
	int  constAddr;  //  Where the first constant is stored.

	// OUTPUT LOCATIONS:
	int  evalPars;  //  Where U(t), for given t, is stored.
	int  evalDPars;  //  Where the derivatives of the parameters are stored.
	int  evalFuncs;  //  Where H(x,t) is stored.
	int  evalJVars;  //  Where the Jacobian w.r.t. vars is stored.
	int  evalJPars;  //  Where the Jacobian w.r.t. pars is stored.
	int  evalSubs;  //  Where the subfunctions are stored
	int  evalJSubsV;  //  Where the derivatives of the subfunctions w.r.t. vars are stored.
	int  evalJSubsP;  //  Where the derivatives of the subfunctions w.r.t. pars are stored.
} prog_t;
*/



namespace bertini {

	class SLPCompiler;
	class System; // a forward declaration, solving the circular inclusion problem


	enum Operation { // we'll start with the binary ones
		Add=      1 << 0,
		Subtract= 1 << 1,
		Multiply= 1 << 2,
		Divide=   1 << 3,
		Power=    1 << 4,
		Exp=      1 << 5,
		Log=      1 << 6,
		Negate=   1 << 7,
		Sin=      1 << 8,
		Cos=      1 << 9,
		Tan=      1 << 10,
		Asin=     1 << 11,
		Acos=     1 << 12,
		Atan=     1 << 13,
		Assign=   1 << 14,
	};

	const int BinaryOperations = Add|Subtract | Multiply|Divide | Power;
	const int TrigOperations   = Sin|Cos|Tan | Asin|Acos|Atan;
	const int UnaryOperations  = Exp|Log | Negate | Assign | TrigOperations;

	constexpr bool IsUnary(Operation op)
	{
		return op & UnaryOperations;
	}

	constexpr bool IsBinary(Operation op)
	{
		return op & BinaryOperations;
	}

	/**
	 \class StraightLineProgram

	 An implementation of straight-line programs, implemented with strong inspiration from Bertini1's implementation.

	 One constructs a SLP from a system, like

	 ```
	 System my_system();
	 StraightLineProgram slp(my_system);
	 ```

	 Maybe you don't need to know this, but in construction the SLP uses a helper class, the SLPCompiler

	 Patches are just functions in this framework.  The variables appear at the front of the memory, then functions, then derivatives.  This should make copying data out easy, because it's all in one place.

	 In contrast to Bertini1 SLP's, we don't put all the numbers at the front -- they just get scattered through the SLP's memory.
	 */
	class StraightLineProgram{
		friend SLPCompiler;

	private:
		using Nd = std::shared_ptr<const node::Node>;

	public:

		/**
		 \struct OutputLocations

		 A struct encapsulating the starting locations of things in the SLP
		 */
		struct OutputLocations{
			size_t Functions{0};
			size_t Variables{0};
			size_t Jacobian{0};
			size_t TimeDeriv{0};
		};

		/**
		 \struct InputLocations

		 A struct encapsulating the starting locations of things in the SLP
		 */
		struct InputLocations{
			size_t Variables{0};
			size_t Time{0};
		};

		/**
		 \struct NumberOf

		 A struct encapsulating the numbers of things appearing in the SLP
		 */
		struct NumberOf{
			size_t Functions{0};
			size_t Variables{0};
			size_t Jacobian{0};
			size_t TimeDeriv{0};
		};

		/**
		The constructor -- how to make a SLP from a System.
		*/
		StraightLineProgram(System const & sys);

		StraightLineProgram() = default;

		template<typename Derived>
		void Eval(Eigen::MatrixBase<Derived> const& variable_values) const
		{

			using NumT = typename Derived::Scalar;
			CopyVariableValues(variable_values);

			Eval<NumT>();


		}

		/**
		\brief copies the variable values into the Matrix base and the path variables into the complex type time

		\tparam Derived derived type

		\tparam ComplexT complex type

		\param variable_values dervied matrixBase of variable values

		\param  time complex type for time

		 */
		template<typename Derived, typename ComplexT>
		void Eval(Eigen::MatrixBase<Derived> const& variable_values, ComplexT const& time) const
		{
			using NumT = typename Derived::Scalar;
			static_assert(std::is_same<NumT, ComplexT>::value, "scalar types must be the same");

			// 1. copy variable values into memory locations they're supposed to go in
			CopyVariableValues(variable_values);
			CopyPathVariable(time);
			Eval<NumT>();
		}


		/**
		\brief loops through the instructions in memory and evaluates each operation

		\tparam NumT numeric type

		uses a switch to find different operations from memory to make sure its performing the correct evaluations

		 */
		template<typename NumT>
		void Eval() const{
			auto& memory =  std::get<std::vector<NumT>>(memory_);
			for (int ii = 0; ii<instructions_.size();/*the increment is done at end of loop depending on arity */) {
				//in the unary case the loop will increment by 3
				//binary: by 4

				switch (instructions_[ii]) {

					case Add:
						memory[this->instructions_[ii+3]] = memory[instructions_[ii+1]] + memory[instructions_[ii+2]];
						break;

					case Subtract:
						memory[this->instructions_[ii+3]] = memory[instructions_[ii+1]] - memory[instructions_[ii+2]];
						break;

					case Multiply:
						memory[this->instructions_[ii+3]] = memory[instructions_[ii+1]] * memory[instructions_[ii+2]];
						break;

					case Divide:
						memory[this->instructions_[ii+3]] = memory[instructions_[ii+1]] / memory[instructions_[ii+2]];
						break;

					case Power:
						memory[this->instructions_[ii+3]] = pow(memory[instructions_[ii+1]], memory[instructions_[ii+2]]);
						break;

					case Assign:
						memory[this->instructions_[ii+2]] = memory[instructions_[ii+1]];
						break;

					case Negate:
						memory[this->instructions_[ii+2]] = -(memory[instructions_[ii+1]]);
						break;

					case Log:
						memory[this->instructions_[ii+2]] = log(memory[instructions_[ii+1]]);
						break;

					case Exp:
						memory[this->instructions_[ii+2]] = exp(memory[instructions_[ii+1]]);
						break;

					case Sin:
						memory[this->instructions_[ii+2]] = sin(memory[instructions_[ii+1]]);
						break;

					case Cos:
						memory[this->instructions_[ii+2]] = cos(memory[instructions_[ii+1]]);
						break;

					case Tan:
						memory[this->instructions_[ii+2]] = tan(memory[instructions_[ii+1]]);
						break;

					case Asin:
						memory[this->instructions_[ii+2]] = asin(memory[instructions_[ii+1]]);
						break;

					case Acos:
						memory[this->instructions_[ii+2]] = acos(memory[instructions_[ii+1]]);
						break;

					case Atan:
						memory[this->instructions_[ii+2]] = atan(memory[instructions_[ii+1]]);
						break;

				}


				if (IsUnary(static_cast<Operation>(instructions_[ii]))) {
					ii = ii+3;

				}
				//in the binary case the loop will increment by 4
				else {
					ii = ii+4;
				}
			}
		}



		/**
		\brief assignts the computed values of functions into the given vector

		\tparam NumT numeric type

		\param result The vector you're going to store the values into

		the function will automatically resize your vector for you to be the correct size

		 */
		template<typename NumT>
		void GetFuncVals(Vec<NumT> & result) const{
			auto& memory =  std::get<std::vector<NumT>>(memory_);
			// 1. make container, size correctly.
			result.resize(number_of_.Functions);
			// 2. copy content
			for (int ii = 0; ii < number_of_.Functions; ++ii) {
				result(ii) = memory[ii + output_locations_.Functions];
			}
		}

		/**
		\brief retrieves the computed values of jacobians

		\tparam NumT numeric type

		\param result The vector you're going to store the values into

		the function will automatically resize your vector for you to be the correct size

		 */

		template<typename NumT>
		void GetJacobian(Mat<NumT> & result) const{
			auto& memory =  std::get<std::vector<NumT>>(memory_);
			// 1. make container, size correctly.
			result.resize(number_of_.Functions, number_of_.Variables);
			// 2. copy content
			for (int jj =0; jj < number_of_.Variables; ++jj) {
				for (int ii = 0; ii < number_of_.Functions; ++ii) {
					result(ii, jj) = memory[ii+jj*number_of_.Functions + output_locations_.Jacobian];
				}
			}
		}

		/**
		\brief copies the values of the time derivatives into your given vector

		\tparam NumT numeric type

		\param result The vector you're going to store the values into

		the function will automatically resize your vector for you to be the correct size

		 */

		template<typename NumT>
		void GetTimeDeriv(Vec<NumT> & result) const{
			auto& memory =  std::get<std::vector<NumT>>(memory_);
			// 1. make container, size correctly.
			result.resize(number_of_.Functions);
			// 2. copy content
			for (int ii = 0; ii < number_of_.Functions; ++ii) {
				result(ii) = memory[ii + output_locations_.TimeDeriv];
			}
		}

		/**
		\brief creates the Vec<NumT> to be used in the overloaded function

		\tparam NumT numeric type

		 */
		template<typename NumT>
		Vec<NumT> GetFuncVals() const{
			Vec<NumT> return_me;
			GetFuncVals(return_me);
			return return_me;
		}
		/**
		\brief creates the Vec<NumT> to be used in the overloaded function

		\tparam NumT numeric type

		 */
		template<typename NumT>
		Mat<NumT> GetJacobian() const{
			Mat<NumT> return_me;
			GetJacobian(return_me);
			return return_me;
		}
		/**
		\brief creates the Vec<NumT> to be used in the overloaded function

		\tparam NumT numeric type

		 */
		template<typename NumT>
		Vec<NumT> GetTimeDeriv() const{
			Vec<NumT> return_me;
			GetTimeDeriv(return_me);
			return return_me;
		}


		inline unsigned NumFunctions() const{ return number_of_.Functions;}

		inline unsigned NumVariables() const{ return number_of_.Variables;}


		/**
		\brief Get the current precision of the SLP.

		\return The number of digits
		*/
		inline
		unsigned precision() const
		{
			return precision_;
		}

		/**
		\brief change the precision of the SLP.

		Downsamples from the true values.

		\param new_precision The new number of digits
		*/
		void precision(unsigned new_precision) const;

		/**
		 \brief Does this SLP have a path variable?

		 \return Well, does it?
		 */
		bool HavePathVariable() const {
			throw std::runtime_error("calling unimplemented function HavePathVariable");
			return false;
		}

		/**
		 \brief Overloaded operator for printing to an arbirtary out stream.
		 */
		friend std::ostream& operator <<(std::ostream& out, const StraightLineProgram & s);


	private:

		/**
		 \brief Copy the values of the variables from the passed in vector to memory

		 \param variable_values The vector of current variable values.
		 */
		template<typename Derived>
		void CopyVariableValues(Eigen::MatrixBase<Derived> const& variable_values) const{
			using NumT = typename Derived::Scalar;
			auto& memory =  std::get<std::vector<NumT>>(memory_); // unpack for local reference

			for (int ii = 0; ii < number_of_.Variables; ++ii) {
				//assign  to memory
				memory[ii + output_locations_.Variables] = variable_values(ii);
			}
		}

		/**
		 \brief Copy the current time value to memory

		 \param time The current time
		 \tparam ComplexT the complex numeric type.

		 If the SLP doesn't have a path variable, then this will throw.
		 */
		template<typename ComplexT>
		void CopyPathVariable(ComplexT const& time) const{
			if (!this->HavePathVariable())
				throw std::runtime_error("calling Eval with path variable, but system doesn't have one.");
			// then actually copy the path variable into where it goes in memory
		}

		/**
		 \brief Add an instruction to memory.  This one's for binary operations

		 \param binary_op The opcode, from the enum.
		 \param in_loc1 The location of the first operand
		 \param in_loc2 The locatiion in memory of the second operand
		 \param out_loc Where in memory to put the result of the operation.
		 */
		void AddInstruction(Operation binary_op, size_t in_loc1, size_t in_loc2, size_t out_loc);

		/**
		 \brief Add an instruction to memory.  This one's for unary operations

		 \param unary_op The opcode, from the enum.
		 \param in_loc The location of the one and only operand
		 \param out_loc Where in memory to put the result of the operation.
		 */
		void AddInstruction(Operation unary_op, size_t in_loc, size_t out_loc);


		/**
		 \brief Add a number to the memory at location, and memoize it for precision changing later.
		 */
		void AddNumber(Nd const num, size_t loc);

		template<typename NumT>
		auto& GetMemory() const{
			return std::get<std::vector<NumT>>(this->memory_);
		}


		template<typename NumT>
		void CopyNumbersIntoMemory() const;


		unsigned precision_ = 0; //< The current working number of digits
		bool has_path_variable_ = false; //< Does this SLP have a path variable?

		NumberOf number_of_;  //< Quantities of things
		OutputLocations output_locations_; //< Where to find outputs, like functions and derivatives
		InputLocations input_locations_; //< Where to find inputs, like variables and time

		mutable std::tuple< std::vector<dbl_complex>, std::vector<mpfr_complex> > memory_; //< The memory of the object.  Numbers and variables, plus temp results and output locations.  It's all one block.  That's why it's called a SLP!

		std::vector<size_t> instructions_; //< The instructions.  The opcodes are  stored as size_t's, as well as the locations of operands and results.
		std::vector< std::pair<Nd,size_t> > true_values_of_numbers_; //< the size_t is where in memory to downsample to.
	};


	class SLPCompiler : public VisitorBase,
			// symbols and roots
			public Visitor<node::Variable>,
			public Visitor<node::Integer>,
			public Visitor<node::Float>,
			public Visitor<node::Rational>,
			public Visitor<node::Function>,
			public Visitor<node::Jacobian>,
			public Visitor<node::Differential>,

			// arithmetic
			public Visitor<node::SumOperator>,
			public Visitor<node::MultOperator>,
			public Visitor<node::IntegerPowerOperator>,
			public Visitor<node::PowerOperator>,
			public Visitor<node::ExpOperator>,
			public Visitor<node::LogOperator>,

			// the trig operators
			public Visitor<node::SinOperator>,
			public Visitor<node::ArcSinOperator>,
			public Visitor<node::CosOperator>,
			public Visitor<node::ArcCosOperator>,
			public Visitor<node::TanOperator>,
			public Visitor<node::ArcTanOperator>


			// also missing -- linears and difflinears.

			// these abstract base types left out,

			// but commented here to explain why
			//    public Visitor<node::Operator>,// abstract
			//    public Visitor<node::UnaryOperator>,// abstract
			//    public Visitor<node::NaryOperator>,// abstract
			//    public Visitor<node::TrigOperator>,// abstract
	{
	private:
		using Nd = std::shared_ptr<const node::Node>;
		using SLP = StraightLineProgram;

		public:

			SLP Compile(System const& sys);

			// symbols and roots
			virtual void Visit(node::Variable const& n);
			virtual void Visit(node::Integer const& n);
			virtual void Visit(node::Float const& n);
			virtual void Visit(node::Rational const& n);
			virtual void Visit(node::Function const& n);
			virtual void Visit(node::Jacobian const& n);
			virtual void Visit(node::Differential const& n);

			// arithmetic
			virtual void Visit(node::SumOperator const& n);
			virtual void Visit(node::MultOperator const& n);
			virtual void Visit(node::IntegerPowerOperator const& n);
			virtual void Visit(node::PowerOperator const& n);
			virtual void Visit(node::ExpOperator const& n);
			virtual void Visit(node::LogOperator const& n);

			// the trig operators
			virtual void Visit(node::SinOperator const& n);
			virtual void Visit(node::ArcSinOperator const& n);
			virtual void Visit(node::CosOperator const& n);
			virtual void Visit(node::ArcCosOperator const& n);
			virtual void Visit(node::TanOperator const& n);
			virtual void Visit(node::ArcTanOperator const& n);

			// missing -- linear and difflinear
		private:


			/**
			 \brief Provides a uniform interface for dealing with all numeric node types.
			 */
			template<typename NodeT>
			void DealWithNumber(NodeT const& n){
						auto nd=n.shared_from_this(); // make a shared pointer to the node, so that it survives, and we get polymorphism
						this->slp_under_construction_.AddNumber(nd, next_available_mem_); // register the number with the SLP
						this->locations_encountered_symbols_[nd] = next_available_mem_++; // add to found symbols in the compiler, increment counter.
			}

			/**
			 \brief Reset the compiler to compile another SLP from another system.
			 */
			void Clear();

			size_t next_available_mem_ = 0; //< Where should the next thing in memory go?
			std::map<Nd, size_t> locations_encountered_symbols_; //< A registry of pointers-to-nodes and location in memory on where to find *their results*
			SLP slp_under_construction_; //< the under-construction SLP.  will be returned at end of `compile`
	};



} // namespace bertini









#endif // for the ifndef include guards
