
#include <regex>

#include "helper_functions.h"

#include <iostream>
#include <fstream>
#include <fmt/ostream.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#include <clang/AST/CXXInheritance.h>
#include "clang/AST/DeclTemplate.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/AST/Comment.h"
#pragma clang diagnostic pop

#include "wrapped_class.h"
#include "output_modules.h"
#include "clang_helper_functions.h"

using namespace v8toolkit::class_parser;

namespace v8toolkit::class_parser {


// remove trailing & or && from the end of a string
string remove_reference_from_type_string(string const & type_string) {
//    std::cerr << fmt::format("removing reference from string: '{}'", type_string) << std::endl;
    auto result = std::regex_replace(type_string, std::regex("\\s*&{1,2}\\s*$"), "");
//    std::cerr << fmt::format("result: '{}'", result) << std::endl;
    return result;

}

string remove_local_const_from_type_string(string const & type_string) {
//    std::cerr << fmt::format("removing const from string: '{}'", type_string) << std::endl;

    auto result = std::regex_replace(type_string, std::regex("^\\s*const\\s*"), "");
//    std::cerr << fmt::format("result: '{}'", result) << std::endl;
    return result;

}


string make_macro_safe_comma(string const & input) {
    return std::regex_replace(input, std::regex("\\s*,\\s*"), " V8TOOLKIT_COMMA ");
}

// Gets the "most basic" type in a type.   Strips off ref, pointer, CV
//   then calls out to get how to include that most basic type definition
//   and puts it in wrapped_class.include_files
void update_wrapped_class_for_type(WrappedClass & wrapped_class,
    // don't capture qualtype by ref since it is changed in this function
                                   QualType const & input_qual_type) {

//    cerr << fmt::format("In update_wrapped_class_for_type {} in wrapped class {}", qual_type.getAsString(), wrapped_class.class_name) << endl;

    if (print_logging) cerr << "Went from " << input_qual_type.getAsString();
    QualType qual_type = input_qual_type.getLocalUnqualifiedType();


    // remove pointers
    while (!qual_type->getPointeeType().isNull()) {
        qual_type = qual_type->getPointeeType();
    }
    qual_type = qual_type.getLocalUnqualifiedType();

    if (print_logging) cerr << " to " << qual_type.getAsString() << endl;
    auto base_type_record_decl = qual_type->getAsCXXRecordDecl();


    if (auto function_type = dyn_cast<FunctionType>(&*qual_type)) {
//        cerr << "IS A FUNCTION TYPE!!!!" << endl;

        // it feels strange, but the type int(bool) from std::function<int(bool)> is a FunctionProtoType
        if (auto function_prototype = dyn_cast<FunctionProtoType>(function_type)) {
//            cerr << "IS A FUNCTION PROTOTYPE" << endl;

//            cerr << "Recursing on return type" << endl;
            update_wrapped_class_for_type(wrapped_class, function_prototype->getReturnType());

            for (auto param : function_prototype->param_types()) {

//                cerr << "Recursing on param type" << endl;
                update_wrapped_class_for_type(wrapped_class, param);
            }
        } else {
//            cerr << "IS NOT A FUNCTION PROTOTYPE" << endl;
        }

    } else {
//        cerr << "is not a FUNCTION TYPE" << endl;
    }


    // primitive types don't have record decls
    if (base_type_record_decl == nullptr) {
        return;
    }

    auto actual_include_string = get_include_for_type_decl(wrapped_class.compiler_instance, base_type_record_decl);

    if (print_logging)
        cerr << &wrapped_class << "Got include string for " << qual_type.getAsString() << ": " << actual_include_string
             << endl;

    // if there's no wrapped type, it may be something like a std::function or STL container -- those are ok to not be wrapped
    if (has_wrapped_class(base_type_record_decl)) {
        auto & used_wrapped_class = WrappedClass::get_or_insert_wrapped_class(base_type_record_decl,
                                                                              wrapped_class.compiler_instance,
                                                                              FOUND_UNSPECIFIED);
        wrapped_class.used_classes.insert(&used_wrapped_class);
    }


    wrapped_class.include_files.insert(actual_include_string);
//    cerr << fmt::format("{} now has {} include files having added {}", wrapped_class.name_alias, wrapped_class.include_files.size(), actual_include_string) << endl;


    if (dyn_cast<ClassTemplateSpecializationDecl>(base_type_record_decl)) {
        if (print_logging) cerr << "##!#!#!#!# Oh shit, it's a template type " << qual_type.getAsString() << endl;

        auto template_specialization_decl = dyn_cast<ClassTemplateSpecializationDecl>(base_type_record_decl);

        // go through the template args
        auto & template_arg_list = template_specialization_decl->getTemplateArgs();
        for (decltype(template_arg_list.size()) i = 0; i < template_arg_list.size(); i++) {
            auto & arg = template_arg_list[i];

            // this code only cares about types, so skip non-type template arguments
            if (arg.getKind() != clang::TemplateArgument::Type) {
                continue;
            }
            auto template_arg_qual_type = arg.getAsType();
            if (template_arg_qual_type.isNull()) {
                if (print_logging) cerr << "qual type is null" << endl;
                continue;
            }
            if (print_logging) cerr << "Recursing on templated type " << template_arg_qual_type.getAsString() << endl;
            update_wrapped_class_for_type(wrapped_class, template_arg_qual_type);
        }
    } else {
        if (print_logging) cerr << "Not a template specialization type " << qual_type.getAsString() << endl;
    }
}


/**
 * Takes a named decl and tries to figure out whether it should be wrapped or not
 */
EXPORT_TYPE get_export_type(const NamedDecl * decl, LogSubjects::Subjects log_subject, EXPORT_TYPE previous) {
    auto & attrs = decl->getAttrs();
    EXPORT_TYPE export_type = previous;

    auto name = decl->getNameAsString();

    bool found_export_specifier = false;

    for (auto attr : attrs) {
        if (dyn_cast<AnnotateAttr>(attr)) {
            auto attribute_attr = dyn_cast<AnnotateAttr>(attr);
            auto annotation_string = attribute_attr->getAnnotation().str();

            if (annotation_string == V8TOOLKIT_ALL_STRING) {
                if (found_export_specifier) {
                    log.error(log_subject, "Found more than one export specifier on {}", name);
                }

                export_type = EXPORT_ALL;
                found_export_specifier = true;
            } else if (annotation_string == V8TOOLKIT_NONE_STRING) {
                if (found_export_specifier) {
                    log.error(log_subject, "Found more than one export specifier on {}", name);
                }
                export_type = EXPORT_NONE; // just for completeness
                found_export_specifier = true;
            }
        }
    }
    log.info(log_subject, "Returning export type: {} for {}", export_type, name);
    return export_type;
}


std::string get_canonical_name_for_decl(const TypeDecl * decl) {
    if (decl == nullptr) {
        llvm::report_fatal_error("null type decl to get_canonical_name_for_decl");
    }
    return decl->getTypeForDecl()->getCanonicalTypeInternal().getAsString();
}

//
//    std::string get_full_name_for_type(QualType qual_type) {
//
//        if (auto typedef_decl = qual_type->getAs<TypedefNameDecl>()) {
//            qual_type = typedef_decl->getUnderlyingType();
//        }
//        auto record_decl = qual_type->getAsCXXRecordDecl();
//        if (record_decl == nullptr) {
//            std::cerr << fmt::format("Couldn't get record decl for {}", qual_type.getAsString()) << std::endl;
//            assert(record_decl != nullptr);
//        }
//
//        return get_canonical_name_for_decl(record_decl);
//    }



//
//QualType get_plain_type(QualType qual_type) {
//    auto type = qual_type.getNonReferenceType();//.getUnqualifiedType();
//    while(!type->getPointeeType().isNull()) {
//        type = type->getPointeeType();//.getUnqualifiedType();
//    }
//    return type;
//}


//
//void generate_bindings() {
//
//    // current file number for bindings file so it can be broken down into multiple files
//    int file_count = 1;
//
//    // start at one so they are never considered "empty"
//    int declaration_count_this_file = 1;
//
//    vector<WrappedClass *> classes_for_this_file;
//
//    set<WrappedClass const *> already_wrapped_classes;
//
//    vector<vector<WrappedClass *>> classes_per_file;
//
//    ofstream js_stub;
//
//
//    cerr << fmt::format("About to start writing out wrapped classes with {} potential classes",
//                        WrappedClass::wrapped_classes.size()) << endl;
//
//
//    // go through all the classes until a full pass has been made with nothing new to be written out
//    bool found_match = true;
//    while (found_match) {
//        found_match = false;
//
//        // go through the list to see if there is anything left to write out
//        for (auto & wrapped_class : WrappedClass::wrapped_classes) {
//
//            cerr << fmt::format("considering dumping class: {}", wrapped_class->class_name) << endl;
//
//            if (!wrapped_class->should_be_wrapped()) {
//                cerr << "should_be_wrapped returned false, not dumping" << endl;
//                continue;
//            }
//
//            // if it has unmet dependencies or has already been mapped, skip it
//            if (!wrapped_class->ready_for_wrapping(already_wrapped_classes)) {
//                std::cerr << fmt::format("Skipping {}", wrapped_class->class_name) << std::endl;
//                continue;
//            }
//            already_wrapped_classes.insert(wrapped_class.get());
//            found_match = true;
//
////            std::cerr << fmt::format("writing class {} to file with declaration_count = {}", wrapped_class->get_name_alias(),
////                                     wrapped_class->declaration_count) << std::endl;
//
//            // if there's room in the current file, add this class
//            auto space_available = declaration_count_this_file == 0 ||
//                                   declaration_count_this_file + wrapped_class->declaration_count <
//                                   MAX_DECLARATIONS_PER_FILE;
//
//            if (!space_available) {
//
//
////                std::cerr << fmt::format("Out of space in file, rotating") << std::endl;
//                classes_per_file.emplace_back(classes_for_this_file);
//
//                // reset for next file
//                classes_for_this_file.clear();
//                declaration_count_this_file = 0;
//                file_count++;
//            }
//
//            classes_for_this_file.push_back(wrapped_class.get());
//
//            // assert false - this shouldn't alter it
//            declaration_count_this_file += wrapped_class->declaration_count;
//        }
//    }
//
//
//
//
//    // if the last file set isn't empty, add that, too
//    if (!classes_for_this_file.empty()) {
//        classes_per_file.emplace_back(classes_for_this_file);
//    }
//
//    if (already_wrapped_classes.size() != WrappedClass::wrapped_classes.size()) {
////        cerr << fmt::format("Could not wrap all classes - wrapped {} out of {}",
////                            already_wrapped_classes.size(), WrappedClass::wrapped_classes.size()) << endl;
//    }
//
//    int total_file_count = classes_per_file.size();
//    for (int i = 0; i < total_file_count; i++) {
//        write_classes(i + 1, classes_per_file[i], i == total_file_count - 1);
//    }


//    cerr << "Classes returned from matchers: " << matched_classes_returned << endl;


//    cerr << "Classes used that were not wrapped:" << endl;
//    for (auto & wrapped_class : WrappedClass::wrapped_classes) {
//        if (!wrapped_class->dumped) { continue; }
//        for (auto used_class : wrapped_class->used_classes) {
//            if (!used_class->dumped) {
//                cerr << fmt::format("{} uses unwrapped type: {}", wrapped_class->get_name_alias(), used_class->get_name_alias())
//                     << endl;
//            }
//        }
//    }
//}



// calls callback for each constructor in the class.  If annotation specified, only
//   constructors with that annotation will be sent to the callback
void foreach_constructor(const CXXRecordDecl * klass, std::function<void(CXXConstructorDecl const *)> callback,
                         const std::string & annotation) {

    if (klass == nullptr) {
        cerr << fmt::format("Skipping foreach_constructor because decl was nullptr") << endl;
        return;
    }

    string class_name = klass->getNameAsString();
    v8toolkit::class_parser::log.info(LogSubjects::Subjects::Enums, "Enumerating constructors for {} with optional annotation: {}", class_name, annotation);

    for (CXXMethodDecl * method : klass->methods()) {
        CXXConstructorDecl * constructor = dyn_cast<CXXConstructorDecl>(method);
        bool skip = false;
        Annotations annotations(method);

        // check if method is a constructor
        if (constructor == nullptr) {
            continue;
        }

        if (print_logging) cerr << "Checking constructor: " << endl;
        if (constructor->getAccess() != AS_public) {
            if (print_logging) cerr << "  Skipping non-public constructor" << endl;
            skip = true;
        }
        if (get_export_type(constructor, LogSubjects::Constructors) == EXPORT_NONE) {
            if (print_logging) cerr << "  Skipping constructor marked for begin skipped" << endl;
            skip = true;
        }

        if (annotation != "" && !annotations.has(annotation)) {
            if (print_logging)
                cerr << "  Annotation " << annotation << " requested, but constructor doesn't have it" << endl;
            skip = true;
        } else {
            if (skip) {
                if (print_logging)
                    cerr << "  Annotation " << annotation
                         << " found, but constructor skipped for reason(s) listed above" << endl;
            }
        }


        if (skip) {
            continue;
        } else {
            if (print_logging) cerr << " Running callback on constructor" << endl;
            callback(constructor);
        }
    }
//    cerr << fmt::format("Done enumerating constructors for {}", class_name) << endl;
}


void generate_bidirectional_classes(CompilerInstance & compiler_instance) {
    cerr << "Generating bidirectional classes..." << endl;

    // use this style loop because entries will be added inside the loop
    for (auto wrapped_class_iterator = WrappedClass::wrapped_classes.begin();
         wrapped_class_iterator != WrappedClass::wrapped_classes.end();
         wrapped_class_iterator++) {
        auto & wrapped_class = *wrapped_class_iterator;


        assert(wrapped_class->base_types.size() == 1);
        auto base_type = *wrapped_class->base_types.begin();

        // create the file for the include, stripping off any "" or <> (should only be "'s)
        ofstream bidirectional_file(regex_replace(wrapped_class->my_include, std::regex("[<>\"]"), ""));

        bidirectional_file << endl << "#pragma once\n\n";


        // need to include all the includes from the parent types because the implementation of this bidirectional
        //   type may need the types for things the parent type .h files don't need (like unique_ptr contained types)
        for (auto & include : wrapped_class->get_base_type_includes()) {
            if (include == "") {
                continue;
            }
            std::cerr
                << fmt::format("for bidirectional {}, adding base type include {}", wrapped_class->get_name_alias(), include)
                << std::endl;
            bidirectional_file << "#include " << include << "\n";
        }

        // std::cerr << fmt::format("done adding base type includes now adding wrapped_class include files") << std::endl;
        for (auto & include : wrapped_class->include_files) {
            if (include == "") {
                continue;
            }
            bidirectional_file << "#include " << include << "\n";
        }
        bidirectional_file << endl; // blank line between includes and class definition

        std::cerr << fmt::format("done with includes, building class and constructor") << std::endl;
        bidirectional_file << fmt::format(
            "class JS{} : public {}, public v8toolkit::JSWrapper<{}> {{\npublic:", // {{ is escaped {
            base_type->get_name_alias(), base_type->get_name_alias(), base_type->get_name_alias()) << endl;

        bidirectional_file
            << fmt::format("    JS{}(v8::Local<v8::Context> context, v8::Local<v8::Object> object,",
                           base_type->get_name_alias()) << endl;
        bidirectional_file << fmt::format("        v8::Local<v8::FunctionTemplate> created_by") << endl;
        if (base_type->bidirectional_constructor == nullptr) {
            llvm::report_fatal_error(fmt::format("bidirectional constructor: {} for class {}",
                                                 (void *) base_type->bidirectional_constructor,
                                                 base_type->get_name_alias()));
        }

        // go through all the parameters for the constructor and create variable names for them
        ClassFunction bidirectional_constructor(*base_type, base_type->bidirectional_constructor);
        int param_position = 1;
        for (auto & parameter : bidirectional_constructor.parameters) {
            bidirectional_file << fmt::format(", {} var{}", parameter.type.get_name(), param_position++);
        }


//        bidirectional_file << fmt::format(") :") << endl;
//
//        //                auto variable_names = generate_variable_names(construtor_parameter_count);
//        auto variable_names = "";/*generate_variable_names(
//            get_method_param_qual_types(compiler_instance, base_type->bidirectional_constructor), true);*/
//        bidirectional_file << fmt::format("      {}({}),", base_type->get_name_alias(), join(variable_names)) << endl;
//        bidirectional_file
//            << fmt::format("      v8toolkit::JSWrapper<{}>(context, object, created_by) {{}}", base_type->get_name_alias())
//            << endl; // {{}} is escaped {}
//        bidirectional_file << endl;
//


        auto * current_inheritance_class = base_type;
        set<string> js_access_virtual_methods_added;

        cerr << fmt::format("building virtual method list for {}", wrapped_class->get_name_alias());
        while (current_inheritance_class) {
            cerr << fmt::format(" ** Inheritance hierarchy class: {} with {} base types",
                                current_inheritance_class->get_name_alias(), current_inheritance_class->base_types.size())
                 << endl;

            for (auto & method : current_inheritance_class->get_member_functions()) {
                if (!method->is_virtual) {
                    std::cerr << fmt::format("Skipping method {} because it's either not virtual",
                                             method->name) << std::endl;
                    continue;
                }

                // this is done correctly lower down, so I think this is not needed but signature includes the class name
                // so it doesn't dedupe virtuals across inheritance hierarchy
//                auto added_method = js_access_virtual_methods_added.find(method->get_signature_string());
//
//                // check if it's already been added at a more derived type
//                if (added_method != js_access_virtual_methods_added.end()) {
//                    std::cerr << fmt::format("skipping {} because it's already been added", method->name) << std::endl;
//                    continue;
//                }
//                std::cerr << fmt::format("adding {} as having been added", method->get_signature_string()) << std::endl;
//                js_access_virtual_methods_added.insert(method->get_signature_string());




                auto bidirectional_virtual_method = method->method_decl;
                auto num_params = bidirectional_virtual_method->getNumParams();
//            printf("Dealing with %s\n", method->getQualifiedNameAsString().c_str());


                stringstream js_access_virtual_method_string;

                //std::cerr << fmt::format("10") << std::endl;
                js_access_virtual_method_string << "  JS_ACCESS_" << num_params
                                                << (bidirectional_virtual_method->isConst() ? "_CONST(" : "(");

                js_access_virtual_method_string << make_macro_safe_comma(method->return_type.get_name()) << ", ";

                //std::cerr << fmt::format("11 - num_params: {}", num_params) << std::endl;
                js_access_virtual_method_string << bidirectional_virtual_method->getName().str();

                if (num_params > 0) {
                    //std::cerr << fmt::format("12") << std::endl;
                    auto types = vector<QualType>{};/*get_method_param_qual_types(wrapped_class->compiler_instance,
                                                             bidirectional_virtual_method); */
                    vector<string> type_names;
                    for (auto & type : types) {
                        type_names.push_back(make_macro_safe_comma(type.getAsString()));
                    }

                    js_access_virtual_method_string << join(type_names, ", ", true);
                }


                //std::cerr << fmt::format("13") << std::endl;
                js_access_virtual_method_string << ");\n";


                if (js_access_virtual_methods_added.count(js_access_virtual_method_string.str())) {
                    continue;
                }
                std::cerr << fmt::format("marking as added: {}", js_access_virtual_method_string.str()) << std::endl;
                js_access_virtual_methods_added.insert(js_access_virtual_method_string.str());

                // do this after marking it as added so if it doesn't get added when found in a parent type of
                //   the current class where it wouldn't be marked as final
                if (method->is_virtual_final) {
                    std::cerr << fmt::format("Skipping method {} because it's marked as final",
                                             method->name) << std::endl;
                    continue;
                }

                bidirectional_file << js_access_virtual_method_string.str();

            }

            current_inheritance_class = *current_inheritance_class->base_types.begin();
        }

        bidirectional_file << endl << "};" << endl << endl;


        bidirectional_file.close();

    }
}

std::string trim_doxygen_comment_whitespace(std::string const & comment) {
    return std::regex_replace(comment, std::regex("\\s*([^\\n]*?)\\s*(?:[\n]|$)\\s*[*]*\\s*?(\\s?)"), "$1$2");
}


/**
 * Returns the FullComment object associated with the specified decl.  Only returns a FullComment if the returned
 * FullComment is attached to the specified Decl, unless `any` is set to true
 * @param decl decl to find an attached comment for
 * @param any specify if comments attached to `related` decls are acceptable to be returned.  For example
 *   a derived class without an attached comment may return its base type's attached comment.  Unless `any` is
 *   true, this situation will result in nullptr being returned from this function
 * @return FullComment * matching the criteria specified or nullptr if no such FullComment was found
 */
FullComment * get_full_comment_for_decl(CompilerInstance & ci, Decl const * decl, bool any) {
    FullComment * full_comment = ci.getASTContext().getCommentForDecl(decl, nullptr);

    if (full_comment != nullptr && (any || full_comment->getDecl() == decl)) {
        return full_comment;
    }

    return nullptr;
}

} // end namespace v8toolkit::class_parser