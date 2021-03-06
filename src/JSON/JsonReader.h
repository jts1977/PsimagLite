//-*-C++-*-

/** \ingroup DCA */
/*@{*/

/*! \file ControlParameters.h
  \brief Contains the parameters relating to the control of algorithms
\author Michael S. Summers
*/

#ifndef DCA_JSON_READER_HEADER_H
#define DCA_JSON_READER_HEADER_H

#include <stdio.h>
#include <string>
#include <vector>
#include <dirent.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>

#include "Whatever.h"
#include "Matrix.h"
#include "Transposer.h"
#include "JsonParser.h"
#include "MatrixParser.h"

namespace dca {

  class JsonReader
  {
  public:

    typedef JsonParser::DefaultContext                 JsonDataType;
    typedef JsonParser::Whatever                       JsonAccessor;
    typedef JsonParser::Whatever::FlatMapType          FlatMapType;

    std::string                          fileName;
    JsonParser::JsonParser<JsonDataType> parser;
    JsonAccessor&                        parseResult;
    FlatMapType                          flatMap;

    //======================================================================

    JsonReader(std::string inputFileName):
      fileName(inputFileName),
      parser(),
      parseResult(parser.ctx.result),
      flatMap    ()
    {
      std::wifstream file(fileName.c_str());
      if (!file || !file.good() || file.bad()) {
	std::ostringstream msg;
	msg << "JsonReader::constructor(): cannot open file " << fileName << "\n";
	throw std::runtime_error(msg.str());
      }

      parser.filename = fileName;		
    
      while(parser.parseChar(file));

      parseResult.flattenInto(flatMap);

      if (false) {
	FlatMapType::const_iterator itr(flatMap.begin());
	for (; itr != flatMap.end(); itr++) {
	  std::wcout << itr->first << "  : "  << itr->second << " " << itr->second->type << L"\n"; // << *(itr->second)
	}
      }
    }

    const JsonAccessor& operator[] (const std::string key) const {
      return parseResult[key];
    }

    const JsonAccessor& operator[] (int index) {
      return parseResult[index];
    }

    int count(const std::string key) const {
      return parseResult.count(key);
    }

    const JsonAccessor& searchFor(const std::string key) const {
      const JsonAccessor& result = searchFlatMapFor(key);
      if (result.type  == JsonAccessor::WHATEVER_NULL) {
	std::ostringstream msg;
	msg << "In JsonReader::searchFor, Could not find key '" << key << "' in the input file!";
	throw std::logic_error(msg.str());
      }
      return result;
    }

    const JsonAccessor& lookFor(const std::string key) const {
      const JsonAccessor& result = searchFlatMapFor(key);
      return result;
    }

    const JsonAccessor& searchFor(const std::string key1,
				  const std::string key2) const {
      const JsonAccessor& result = searchFlatMapFor(key1);
      if (result.type  != JsonAccessor::WHATEVER_NULL)
	return result;
      const JsonAccessor& result2 = searchFlatMapFor(key2);
      if (result2.type  == JsonAccessor::WHATEVER_NULL) {
	std::ostringstream msg;
	msg << "In JsonReader::searchFor, Could not find keys '" << key1 << "' '" << key2 << "' in the input file!";
	throw std::logic_error(msg.str());
      }
      return result2;
    }

    //====================================================================== map[]
    
    const JsonAccessor& searchFlatMapFor(const std::string key) const {

      std::wstring wKey(key.begin(), key.end());

      FlatMapType::const_iterator itr = flatMap.begin();

      for (; itr != flatMap.end(); itr++) {

	const std::wstring& candidateKey = itr->first;

	size_t pos = candidateKey.rfind(wKey);
	// does the candidate key contain wKey?
	if (pos == std::string::npos) 
	  continue; // wKey not found skip it

	// candidate begins with key
	if (pos == 0) { 
	  if (candidateKey.length() == wKey.length() or
	      candidateKey[pos+wKey.length()] == L':') {
	    return *(itr->second);
	  }
	  else {
	    continue;
	  }
	}

	//candidate ends with key
	if (pos == (candidateKey.length() - wKey.length())) {
	  if (candidateKey[pos-1] == L':') 
	    return *(itr->second);
	  continue;
	}
	//  key in middle of candidate
// 	if (candidateKey[pos-1] == L':' and
// 	    candidateKey[pos+wKey.length()] == L':') 
// 	  return pair.second;
      }
      return JsonAccessor::null();
    }

  };

  //======================================================================

  template<typename MatrixLikeType>
  psimag::Transposer<MatrixLikeType>& operator <= 
  (psimag::Transposer<MatrixLikeType>& lhs, 
   const JsonParser::Whatever& w) {
    
    if (w.type == JsonParser::Whatever::WHATEVER_MAP) {
      
      int in_rows;  in_rows <= w["rows"];
      int in_cols;  in_cols <= w["cols"];
      
      if ( lhs.n_row() > 0 or lhs.n_col() > 0) {
	
	// They must be the same:
	if (int(lhs.n_row()) != in_rows or 
	    int(lhs.n_col()) != in_cols) {
	  std::ostringstream msg;
	  msg << "JsonReader => Transposer<matrix>: size mis-match!\n";
	  msg << " Attempt to read a "
	      << "(" << in_rows << "," << in_cols << ") matrix into a "
	      << "(" << lhs.n_row() << "," << lhs.n_col() << ") matrix!\n";
	  msg << "\nNote: If the target transposed matrix has zero rows and columns then the reader \n"
	      << "      will set the target matrix to the size of the input matrix!\n";
	  throw std::logic_error(msg.str());
	}
      }
      else 
	lhs.mat.resize(in_cols, in_rows);
      
      lhs <= w["data"];
      
      return lhs;
    }
    
    loadMatrixLikeFromFile(lhs,w);

    return lhs;
  }

  //======================================================================

  template <typename MatrixLikeType> 
  void loadMatrixLikeFromFile(MatrixLikeType& mat,
			      const JsonParser::Whatever& w) {
    
    if (w.type == JsonParser::Whatever::WHATEVER_MAT) {
      
      std::ostringstream msg;
      msg << "Whatever.loadMatrixLikeFromFile(" << mat.n_row() << "," << mat.n_col()  << ")\n";
      
      std::wifstream file(w.filename.c_str());
      
      if (! file.is_open()) {
	enum {BUFF_SIZE=300};
	char buffer[BUFF_SIZE];
	std::ostringstream msg;
	msg << "JsonReader =>Transposer could not find file " << w.filename << "!\n";
	msg << " The current directory is: " << getcwd(buffer,BUFF_SIZE) << "\n";
	throw std::logic_error(msg.str());
      }
      
      file.seekg(w.startPos);

      JsonParser::MatrixParser< MatrixLikeType > mParser(file,w.endPos,mat);

      return;
    }
    
    if (w.type == JsonParser::Whatever::WHATEVER_VECTOR) {

      // Note that we resize if the matrix size is zero
      if (mat.size() == 0) {  
	size_t rows =  w.whateverVector.size();
	size_t cols =  w.whateverVector[0].size();
	mat.resize(rows,cols);
      }

      for(size_t i=0; i< mat.n_row(); i++) {
	const JsonParser::Whatever& row = w.whateverVector[i];
	for(size_t j=0; j< mat.n_col(); j++) {
	  const JsonParser::Whatever& value = row[j];
	  mat(i,j) <= value;
	}
      }
      return;
    }

    throw std::logic_error("mat <= Whatever error wrong type!");
  }

  //======================================================================
  
  template<typename T,template<typename> class MatrixTemplate>
  MatrixTemplate<T>& operator <= 
  (MatrixTemplate<T>& lhs, 
   const JsonParser::Whatever& w) {
    
    if (w.type == JsonParser::Whatever::WHATEVER_MAP) {
      
      int in_rows;  in_rows <= w["rows"];
      int in_cols;  in_cols <= w["cols"];
      
      if ( lhs.n_row() > 0 or lhs.n_col() > 0) {
	
	// They must be the same:
	if (int(lhs.n_row()) != in_rows or 
	    int(lhs.n_col()) != in_cols) {
	  std::ostringstream msg;
	  msg << "JsonReader => matrix: Matrix size mis-match!\n";
	  msg << " Attempt to read a "
	      << "(" << in_rows << "," << in_cols << ") matrix into a "
	      << "(" << lhs.n_row() << "," << lhs.n_col() << ") matrix!\n";
	  msg << "\nNote: If the target matrix has zero rows and columns then the reader \n"
	      << "      will set the target matrix to the size of the input matrix!\n";
	  throw std::logic_error(msg.str());
	}
      }
      else 
	lhs.resize(in_rows, in_cols);

      lhs <= w["data"];
      
      return lhs;
    }

    if (w.type == JsonParser::Whatever::WHATEVER_MAT) {
      
      MatrixTemplate<T>& mat(lhs);

      std::ostringstream msg;
      msg << "Whatever.loadMatrix(" << mat.n_row() << "," << mat.n_col()  << ")\n";
      
      std::wifstream file(w.filename.c_str());
      
      if (! file.is_open()) {
	enum {BUFF_SIZE=300};
	char buffer[BUFF_SIZE];
	std::ostringstream msg;
	msg << "JsonReader => matrix could not find file " << w.filename << "!\n";
	msg << " The current directory is: " << getcwd(buffer,BUFF_SIZE) << "\n";
	throw std::logic_error(msg.str());
      }
      
      file.seekg(w.startPos);

      JsonParser::MatrixParser<MatrixTemplate<T> > mParser(file,w.endPos,mat);

      return lhs;
    }

    if (w.type == JsonParser::Whatever::WHATEVER_VECTOR) {
      if (lhs.n_row() == 0 || lhs.n_col()==0 ) {
	size_t rows =  w.whateverVector.size();
	size_t cols =  w.whateverVector[0].size();
	lhs.resize(rows,cols);
      }
      for(size_t i=0; i< lhs.n_row(); i++) {
	const JsonParser::Whatever& row = w.whateverVector[i];
	for(size_t j=0; j< lhs.n_col(); j++) {
	  const JsonParser::Whatever& value = row[j];
	  lhs(i,j) <= value;
	}
      }
      return lhs;
    }
    throw std::logic_error("mat <= Whatever error wrong type!");
  }


} // end namespace DCA

/*@}*/

#endif

//#include "ParametersFromJson.h"
