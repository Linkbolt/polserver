//  (C) Copyright John Maddock 2004. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//
// This progam scans for *.ipp files in the libs/config/test
// directory and then generates the *.cpp test files from them
// along with config_test.cpp and a Jamfile.
//

#include <boost/regex.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/test/included/prg_exec_monitor.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <set>
#include <ctime>

namespace fs = boost::filesystem;

fs::path config_path;

std::string copyright(
"//  Copyright John Maddock 2002-4.\n"
"//  Use, modification and distribution are subject to the \n"
"//  Boost Software License, Version 1.0. (See accompanying file \n"
"//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)\n"
"\n"
"//  See http://www.boost.org/libs/config for the most recent version."
"//\n//  Revision $Id: generate.cpp 73153 2011-07-16 20:12:46Z eric_niebler $\n//\n");

std::stringstream config_test1;
std::stringstream config_test1a;
std::stringstream config_test2;
std::stringstream jamfile;
std::stringstream jamfile_v2;
std::set<std::string> macro_list;


void write_config_info()
{
   // load the file into memory so we can scan it:
   fs::ifstream ifs(config_path / "config_info.cpp");
   std::string file_text;
   std::copy(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>(), std::back_inserter(file_text));
   ifs.close();
   // create macro list:
   std::stringstream ss;
   for(std::set<std::string>::const_iterator i(macro_list.begin()), j(macro_list.end());
      i != j;
      ++i)
   {
      ss << "   PRINT_MACRO(" << *i << ");\n";
   }
   std::string macros = ss.str();
   // scan for Boost macro block:
   boost::regex re("BEGIN\\s+GENERATED\\s+BLOCK\\s+DO\\s+NOT\\s+EDIT\\s+THIS[^\\n]+\\n(.*?)\\n\\s+//\\s*END\\s+GENERATED\\s+BLOCK");
   boost::smatch what;
   if(boost::regex_search(file_text, what, re))
   {
      std::string new_text;
      new_text.append(what.prefix().first, what[1].first);
      new_text.append(macros);
      new_text.append(what[1].second, what.suffix().second);
      fs::ofstream ofs(config_path / "config_info.cpp");
      ofs << new_text;
   }
}

void write_config_test()
{
   fs::ofstream ofs(config_path / "config_test.cpp");
   time_t t = std::time(0);
   ofs << "//  This file was automatically generated on " << std::ctime(&t);
   ofs << "//  by libs/config/tools/generate.cpp\n" << copyright << std::endl;
   ofs << "// Test file for config setup\n"
      "// This file should compile, if it does not then\n"
      "// one or more macros need to be defined.\n"
      "// see boost_*.ipp for more details\n\n"
      "// Do not edit this file, it was generated automatically by\n\n"
      "#include <boost/config.hpp>\n#include <iostream>\n#include \"test.hpp\"\n\n"
      "int error_count = 0;\n\n";
   ofs << config_test1.str() << std::endl;
   ofs << config_test1a.str() << std::endl;
   ofs << "int main( int, char *[] )\n{\n" << config_test2.str() << "   return error_count;\n}\n\n";
}

void write_jamfile_v2() 
{
   fs::ofstream ofs(config_path / "all" / "Jamfile.v2");
   time_t t = std::time(0);
   ofs << "#\n# Regression test Jamfile for boost configuration setup.\n# *** DO NOT EDIT THIS FILE BY HAND ***\n"
      "# This file was automatically generated on " << std::ctime(&t);
   ofs << "#  by libs/config/tools/generate.cpp\n"
      "# Copyright John Maddock.\n"
      "# Use, modification and distribution are subject to the \n"
      "# Boost Software License, Version 1.0. (See accompanying file \n"
      "# LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)\n"
      "#\n# If you need to alter build preferences then set them in\n"
      "# the template defined in options_v2.jam.\n#\n"
      "path-constant DOT : . ;\n"
      "include $(DOT)/options_v2.jam ;\n\n"
      "run ../config_info.cpp  : : : <threading>single <toolset>msvc:<runtime-link>static <toolset>msvc:<link>static ;\n"
      "run ../config_info.cpp  : : : <threading>multi : config_info_threaded ;\n"
      "run ../math_info.cpp : : : <toolset>borland:<runtime-link>static <toolset>borland:<link>static ;\n"
      "run ../config_test.cpp : : : <threading>single <toolset>msvc:<runtime-link>static <toolset>msvc:<link>static ;\n"
      "run ../config_test.cpp : : : <threading>multi : config_test_threaded ;\n"
      "run ../limits_test.cpp ../../../test/build//boost_test_exec_monitor ;\n"
      "run ../abi/abi_test.cpp ../abi/main.cpp  ;\n\n";
   ofs << jamfile_v2.str() << std::endl;

}

void write_test_file(const fs::path& file, 
                     const std::string& macro_name, 
                     const std::string& namespace_name, 
                     const std::string& header_file,
                     bool positive_test, 
                     bool expect_success)
{
   if(!fs::exists(file))
   {
      std::cout << "Writing test file " << file.string() << std::endl;

      fs::ofstream ofs(file);
      std::time_t t = std::time(0);
      ofs << "//  This file was automatically generated on " << std::ctime(&t);
      ofs << "//  by libs/config/tools/generate.cpp\n" << copyright << std::endl;
      ofs << "\n// Test file for macro " << macro_name << std::endl;

      if(expect_success)
      {
         ofs << "// This file should compile, if it does not then\n"
            "// " << macro_name << " should ";
         if(positive_test)
            ofs << "not ";
         ofs << "be defined.\n";
      }
      else
      {
         ofs << "// This file should not compile, if it does then\n"
            "// " << macro_name << " should ";
         if(!positive_test)
            ofs << "not ";
         ofs << "be defined.\n";
      }
      ofs << "// See file " << header_file << " for details\n\n";

      ofs << "// Must not have BOOST_ASSERT_CONFIG set; it defeats\n"
         "// the objective of this file:\n"
         "#ifdef BOOST_ASSERT_CONFIG\n"
         "#  undef BOOST_ASSERT_CONFIG\n"
         "#endif\n\n";

      static const boost::regex tr1_exp("BOOST_HAS_TR1.*");

      ofs << "#include <boost/config.hpp>\n";

      if(regex_match(macro_name, tr1_exp))
         ofs << "#include <boost/tr1/detail/config.hpp>\n";

      ofs << "#include \"test.hpp\"\n\n"
         "#if";
      if(positive_test != expect_success)
         ofs << "n";
      ofs << "def " << macro_name << 
         "\n#include \"" << header_file << 
         "\"\n#else\n";
      if(expect_success)
         ofs << "namespace " << namespace_name << " = empty_boost;\n";
      else
         ofs << "#error \"this file should not compile\"\n";
      ofs << "#endif\n\n";

      ofs << "int main( int, char *[] )\n{\n   return " << namespace_name << "::test();\n}\n\n";  
   }
   else
   {
      std::cout << "Skipping existing test file " << file.string() << std::endl;
   }
}

void process_ipp_file(const fs::path& file, bool positive_test)
{
   std::cout << "Info: Scanning file: " << file.string() << std::endl;

   // our variables:
   std::string file_text;
   std::string macro_name;
   std::string namespace_name;
   fs::path positive_file;
   fs::path negative_file;

   // load the file into memory so we can scan it:
   fs::ifstream ifs(file);
   std::copy(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>(), std::back_inserter(file_text));
   ifs.close();
   // scan for the macro name:
   boost::regex macro_regex("//\\s*MACRO\\s*:\\s*(\\w+)");
   boost::smatch macro_match;
   if(boost::regex_search(file_text, macro_match, macro_regex))
   {
      macro_name = macro_match[1];
      macro_list.insert(macro_name);
      namespace_name = boost::regex_replace(file_text, macro_regex, "\\L$1", boost::format_first_only | boost::format_no_copy);
   }
   if(macro_name.empty())
   {
      std::cout << "Error: no macro definition found in " << file.string();
   }
   else
   {
      std::cout << "Info: Macroname: " << macro_name << std::endl;
   }

   // get the output filesnames:
   boost::regex file_regex("boost_([^.]+)\\.ipp");
   positive_file = file.branch_path() / boost::regex_replace(file.leaf().string(), file_regex, "$1_pass.cpp");
   negative_file = file.branch_path() / boost::regex_replace(file.leaf().string(), file_regex, "$1_fail.cpp");
   write_test_file(positive_file, macro_name, namespace_name, file.leaf().string(), positive_test, true);
   write_test_file(negative_file, macro_name, namespace_name, file.leaf().string(), positive_test, false);
   
   // always create config_test data,
   // positive and negative tests go to separate streams, because for some
   // reason some compilers choke unless we put them in a particular order...
   std::ostream* pout = positive_test ? &config_test1a : &config_test1;
   *pout << "#if";
   if(!positive_test)
      *pout << "n";
   *pout << "def " << macro_name 
      << "\n#include \"" << file.leaf().string() << "\"\n#else\nnamespace "
      << namespace_name << " = empty_boost;\n#endif\n";

   config_test2 << "   if(0 != " << namespace_name << "::test())\n"
      "   {\n"
      "      std::cerr << \"Failed test for " << macro_name << " at: \" << __FILE__ << \":\" << __LINE__ << std::endl;\n"
      "      ++error_count;\n"
      "   }\n";

   // always generate the jamfile data:
   jamfile << "test-suite \"" << macro_name << "\" : \n"
      "[ run " << positive_file.leaf().string() << " <template>config_options ]\n"
      "[ compile-fail " << negative_file.leaf().string() << " <template>config_options ] ;\n";

   jamfile_v2 << "test-suite \"" << macro_name << "\" : \n"
      "[ run ../" << positive_file.leaf().string() << " ]\n"
      "[ compile-fail ../" << negative_file.leaf().string() << " ] ;\n";

}

int cpp_main(int argc, char* argv[])
{
   //
   // get the boost path to begin with:
   //
   if(argc > 1)
   {
      fs::path p(argv[1]);
      config_path = p / "libs" / "config" / "test" ;
   }
   else
   {
      // try __FILE__:
      fs::path p(__FILE__);
      config_path = p.branch_path().branch_path() / "test";
   }
   std::cout << "Info: Boost.Config test path set as: " << config_path.string() << std::endl;

   // enumerate *.ipp files:
   boost::regex ipp_mask("boost_(?:(has)|no).*\\.ipp");
   boost::smatch ipp_match;
   fs::directory_iterator i(config_path), j;
   while(i != j)
   {
      if(boost::regex_match(i->path().leaf().string(), ipp_match, ipp_mask))
      {
         process_ipp_file(*i, ipp_match[1].matched);
      }
      ++i;
   }
   write_config_test();
   write_jamfile_v2();
   write_config_info();
   return 0;
}

