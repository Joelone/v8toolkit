
#include <iostream>
#include <fmt/ostream.h>
#include <vector>
#include <set>

#include <xl/library_extensions.h>
#include <xl/templates.h>
using xl::templates::ProviderPtr;


#include "../wrapped_class.h"
#include "../ast_action.h"

#include "../helper_functions.h"
#include "bindings_output.h"

using namespace xl::templates;

namespace v8toolkit::class_parser::bindings_output {

extern Template file_template;
extern Template class_template;
extern Template standard_includes_template;


extern std::map<string, Template> bindings_templates;

bool BindingsCriteria::operator()(WrappedClass const & c) {
    log.info(LogSubjects::BindingsOutput, "Checking criteria for {}", c.class_name);
    // if (c.bidirectional) {
    //     log.info(LogSubjects::Subjects::BindingsOutput, "Skipping generation of bindings for {} because it's a bidirectional type", c.get_name_alias());
    //     return false;
    // }

    return true;
}


std::ostream & BindingsOutputStreamProvider::get_class_collection_stream() {
    this->count++;

    log.info(LogSubjects::BindingsOutput, "Starting bindings output file: {}", this->count);

    string class_wrapper_filename = fmt::format("v8toolkit_generated_class_wrapper_{}.cpp", this->count);

    this->output_stream.close();
    this->output_stream.open(class_wrapper_filename);

    return this->output_stream;
}


struct BindingsProviderContainer;

using P = xl::templates::DefaultProviders<BindingsProviderContainer>;


struct BindingsProviderContainer {

    static ProviderPtr get_provider(Enum::Element const & e) {
        return P::make_provider(
            std::pair("name", e.name),
            std::pair("value", fmt::format("{}", e.value))
        );
    }


    static ProviderPtr get_provider(Enum const & e) {
        return P::make_provider(
            std::pair("name", e.name),
            std::pair("elements", e.elements)
        );
    }




    static ProviderPtr get_provider(WrappedClass const & c) {
        log.info(LogSubjects::BindingsOutput, "get_provider WrappedClass: {}", c.class_name);

        std::vector<MemberFunction const *> call_operator_vector;
        if (c.call_operator_member_function) {
            call_operator_vector.push_back(c.call_operator_member_function.get());
        }


        auto provider = P::make_provider(
//            std::pair("class", std::ref(c)), // ability to call another template on the same object
            std::pair("comment", c.comment),
            std::pair("name", c.class_name),
            std::pair("long_name", c.class_name),
            std::pair("js_name", c.get_js_name()),
            std::pair("data_members", c.get_members()),
            std::pair("member_functions", std::ref(c.get_member_functions())),
            std::pair("call_operator", call_operator_vector), // either empty vector or contains 1 element
            std::pair("static_functions", std::ref(c.get_static_functions())),
            std::pair("enums", c.get_enums()),
            std::pair("wrapper_extension_methods", c.wrapper_extension_methods),
            std::pair("constructor", c.get_constructors().size() == 0 || c.force_no_constructors ?
                                     fmt::format("class_wrapper.expose_static_methods(\"{}\", isolate);",
                                                 c.get_js_name()) :
                                     fmt::format(
                                         "class_wrapper.add_constructor<{}>(\"{}\", isolate, {});",
                                         c.get_constructors().back()->get_parameter_types_string(),
                                         c.get_js_name(), c.get_constructors().back()->get_default_argument_tuple_string())),
            std::pair("base_type_name", c.base_types.empty() ? "" : (*c.base_types.begin())->class_name),

//            std::pair("custom_extensions", c.foreach_inheritance_level<std::vector<std::string>>(
//                [](auto & c, auto extensions){
//                    extensions.insert(extensions.end(), c.wrapper_custom_extensions.begin(), c.wrapper_custom_extensions.end());
//                    return extensions;
//                })
//            ),

            // if you want the custom extension on the derived type, just "using" it in - this allows for
            //   more flexibility in times when you may not want it
            std::pair("custom_extensions", c.wrapper_custom_extensions.empty() ? "" : *c.wrapper_custom_extensions.begin()),

            // convert to string because it may be a bidirectional type.   Full WrappedClass information isn't
            //   available for them.
//            xl::forward_as_pair("derived_types", xl::transform_if(c.derived_types, [](WrappedClass * c)->std::optional<std::string>{return c->get_name_alias();}))
            xl::forward_as_pair("derived_types", xl::transform(c.derived_types, [](WrappedClass * c){return c->class_name;}))
        );

        return provider;
    }


    static std::string make_member_parameter_string(DataMember const & data_member) {
        std::string result = "";
        if (data_member.accessed_through == nullptr) {
            result = fmt::format("&{}", data_member.long_name);
        } else {
            if (data_member.accessed_through->accessed_through != nullptr) {
                log.error(LogT::Subjects::BindingsOutput, "Bindings output doesn't support multi-level PIMPL members: {}", data_member.long_name);
            } else {
                result = fmt::format("&{}, &{}", data_member.accessed_through->long_name, data_member.long_name);
            }
        }
        std::cerr << fmt::format("For {}, returning add_member template parameter: {}", data_member.long_name, result) << std::endl;
        return result;
    }

    static ProviderPtr get_provider(DataMember const & d) {
        log.info(LogSubjects::BindingsOutput, "get_provider DataMember: {}", d.long_name);

        return P::make_provider(
            std::pair("comment", d.comment),
            std::pair("name", d.long_name),
            std::pair("js_name", d.js_name),
            std::pair("declared_in", d.declared_in.class_name),
            std::pair("type", d.type.get_name()),
            std::pair("read_only", d.is_const ? "_readonly" : ""),
            std::pair("member_pointer", make_member_parameter_string(d))
        );
    }


    static ProviderPtr get_provider(MemberFunction const & f) {
        log.info(LogSubjects::BindingsOutput, "get_provider MemberFunction: {}", f.name);

        return P::make_provider(
            std::pair("js_name", f.js_name),
            std::pair("name", fmt::format("static_cast<{}({}::*)({}) {} {} {} {} {}>(&{})", 
                                          f.return_type.get_name(), f.wrapped_class.class_name, f.get_parameter_types_string(), 
                                          f.is_const() ? "const" : "",
                                          f.is_volatile() ? "volatile" : "",
                                          f.is_lvalue_qualified() ? "&" : "",
                                          f.is_rvalue_qualified() ? "&&" : "",
                                          f.get_exception_specifier_string() == "noexcept" ? "noexcept" : "", f.name)),
            std::pair("comment", f.comment),
            std::pair("binding_parameters", f.get_return_and_class_and_parameter_types_string()),
            std::pair("parameters", P::make_provider(f.parameters)),
            std::pair("return_type_name", f.return_type.get_jsdoc_type_name()),
            std::pair("return_comment", f.return_type_comment),
            std::pair("class_name", f.wrapped_class.class_name),
            std::pair("default_arg_tuple", f.get_default_argument_tuple_string())
        );
    }


    static ProviderPtr get_provider(StaticFunction const & f) {
        log.info(LogSubjects::BindingsOutput, "get_provider StaticFunction: {}", f.name);

        return P::make_provider(
            std::pair("name", f.name),
            std::pair("js_name", f.js_name),
            std::pair("comment", f.comment),
            std::pair("binding_parameters", f.get_return_and_parameter_types_string()),
            std::pair("parameters", P::make_provider(f.parameters)),
            std::pair("return_type_name", f.return_type.get_jsdoc_type_name()),
            std::pair("return_comment", f.return_type_comment),
            std::pair("class_name", f.wrapped_class.class_name),
            std::pair("default_arg_tuple", f.get_default_argument_tuple_string())
        );
    }


    static ProviderPtr get_provider(ClassFunction::ParameterInfo const & p) {
        log.info(LogSubjects::BindingsOutput, "get_provider ParameterInfo: {}", p.name);

        return P::make_provider(
            std::pair("type", p.type.get_jsdoc_type_name()),
            std::pair("name", p.name),
            std::pair("comment", p.description)
        );
    }


    static ProviderPtr get_provider(TypeInfo const & t) {
        return P::make_provider("Implement me");
    }

}; // end BindingsProviderContainer




struct BindingFile {
    size_t max_declaration_count;
    size_t declaration_count = 0;
    std::set<std::string> includes;


    BindingFile(size_t max_declaration_count) :
        max_declaration_count(max_declaration_count)
    {

    }

    /**
     * Whether this binding file can hold the specified WrappedClass
     * @param wrapped_class WrappedClass to see if this BindingFile can hold it
     * @return true if it can be held without exceeding declaration limit, false otherwise
     */
    bool can_hold(WrappedClass const * wrapped_class) {
        return declaration_count == 0 ||
               max_declaration_count == 0 ||
               this->declaration_count + wrapped_class->declaration_count <= max_declaration_count;
    }

    std::vector<WrappedClass const *> classes;

    // this is currently unused - it's not the clear win that it used to be
    std::set<WrappedClass const *> extern_templates;
    std::set<WrappedClass const *> explicit_instantiations;
    std::set<WrappedClass const *> explicit_instantiations_for_const_types;

    // classes with private members to expose need to have WrapperBuilder<> specialized
    std::set<WrappedClass const *> class_needs_wrapper_builder_specialization;

    std::vector<WrappedClass const *> const & get_classes() const {return this->classes;}

    void add_class(WrappedClass const * wrapped_class) {
        std::cerr << fmt::format("adding to BindingFile: {} at address {}", wrapped_class->class_name, (void*)wrapped_class) << std::endl;
        this->classes.push_back(wrapped_class);
        this->declaration_count += wrapped_class->declaration_count;
        assert(this->max_declaration_count == 0 ||  // unlimited
                   this->declaration_count <= this->max_declaration_count || // 1 or more classes fit under the limit
                   this->classes.size() == 1); // if a single class doesn't fit, then force it through

        this->explicit_instantiations.insert(wrapped_class);
        if (!wrapped_class->wrapper_extension_methods.empty()) {
            this->explicit_instantiations_for_const_types.insert(wrapped_class);          
        }        
        this->includes.insert(wrapped_class->include_files.begin(), wrapped_class->include_files.end());
        if (wrapped_class->has_pimpl_members()) {
            this->class_needs_wrapper_builder_specialization.insert(wrapped_class);
        }
    }
};


std::vector<std::string> get_class_body(std::vector<WrappedClass const *> classes) {
    std::vector<std::string> results;
    for (auto c : classes) {
        if (c->has_pimpl_members()) {
            results.emplace_back(fmt::format("v8toolkit::WrapperBuilder<{}>()(isolate);", c->class_name));
        } else {
            results.emplace_back(bindings_templates["class"].fill<BindingsProviderContainer>(*c));
        }
    }
    return results;
}


void BindingsOutputModule::process(std::vector<WrappedClass const*> wrapped_classes)
{
    std::vector<BindingFile> binding_files{BindingFile(this->max_declarations_per_file)};
    std::set<WrappedClass const *> already_wrapped_classes;


    // go through all the classes until a full pass has been made with nothing new to be written out
    bool found_match = true;
    while (found_match) {
        found_match = false;

        // go through the list to see if there is anything left to write out
        for (auto & wrapped_class : wrapped_classes) {

            // if it has unmet dependencies or has already been mapped, skip it
            if (!wrapped_class->ready_for_wrapping(already_wrapped_classes)) {
//                std::cerr << fmt::format("Skipping because not ready_for_wrapping: {}", wrapped_class->class_name) << std::endl;
                continue;
            }

            log.info(LogSubjects::BindingsOutput, "Processing class: {}", wrapped_class->class_name);
            already_wrapped_classes.insert(wrapped_class);
            found_match = true;


            if (!binding_files.back().can_hold(wrapped_class)) {

                log.info(LogSubjects::BindingsOutput, "Bindings File full, rotating");
                binding_files.emplace_back(this->max_declarations_per_file);
            }

            binding_files.back().add_class(wrapped_class);

            // assert false - this shouldn't alter it
            already_wrapped_classes.insert(wrapped_class);
        }
    }


    if (already_wrapped_classes.size() != wrapped_classes.size()) {
//        cerr << fmt::format("Could not wrap all classes - wrapped {} out of {}",
//                            already_wrapped_classes.size(), wrapped_classes.size()) << endl;
    }


//    for (auto const & [binding_file, i] : xl::each_i(binding_files)) {
    for (int i = 0; i < binding_files.size(); i++) {

        auto & binding_file = binding_files[i];


        bool last_file = i == binding_files.size() - 1;
        int file_number = i + 1;

        // this takes care of providing the correct stream for each subsequent call
        auto & output_stream = this->output_stream_provider->get_class_collection_stream();
//        std::cerr << fmt::format("bindings_templates[file] contents: {}", bindings_templates["file"].c_str()) << std::endl;
        auto template_result = bindings_templates["file"].fill<BindingsProviderContainer>(
            P::make_provider(
                std::pair("file_number", fmt::format("{}", file_number)),
                std::pair("next_file_number", fmt::format("{}", file_number + 1)), // ok if it won't actually exist
                std::pair("pimpl_classes", P::make_provider(std::ref(binding_file.class_needs_wrapper_builder_specialization))),
                std::pair("classes", get_class_body(binding_file.get_classes())),
                std::pair("includes", P::make_provider(binding_file.includes)),
                std::pair("extern_templates", binding_file.extern_templates),
                std::pair("explicit_instantiations", binding_file.explicit_instantiations),
                std::pair("explicit_instantiations_for_const_types", binding_file.explicit_instantiations_for_const_types),
                std::pair("call_next_function", !last_file ? fmt::format("v8toolkit_initialize_class_wrappers_{}(isolate);", file_number + 1) : "")
            ),
            &bindings_templates
        );

        log.info(LogSubjects::Subjects::BindingsOutput, "Writing binding file {}", file_number);
        log.info(LogSubjects::Subjects::BindingsOutput, template_result);
        output_stream << template_result << std::flush;
    }

//    cerr << "Classes used that were not wrapped:" << endl;
    for (auto & wrapped_class : wrapped_classes) {
        if (!xl::contains(already_wrapped_classes, wrapped_class)) {
            for (auto used_class : wrapped_class->used_classes) {
                if (already_wrapped_classes.count(used_class) == 0) {
                    log.error(LogSubjects::Class, "Could not dump '{}' because it uses type '{}' that wasn't dumped", wrapped_class->class_name,
                                        used_class->class_name);
                }
            }
        }
    }
}



BindingsOutputModule::BindingsOutputModule(size_t max_declarations_per_file,
                                           unique_ptr<OutputStreamProvider> output_stream_provider) :
    OutputModule(std::move(output_stream_provider))
{

    // prefer the config file
    if (auto maybe_max_declarations_per_file = PrintFunctionNamesAction::get_config_data()["output_modules"]["BindingsOutputModule"]["max_declarations_per_file"].get_number()) {
        if (*maybe_max_declarations_per_file < 0) {
            throw ClassParserException("Config file BindingsOutputModule max_declarations_per_file ({}) must be non-negative", *maybe_max_declarations_per_file);
        }
        this->max_declarations_per_file = *maybe_max_declarations_per_file;
    } else if (max_declarations_per_file != -1) {
        this->max_declarations_per_file = max_declarations_per_file;
    }
    log.info(LogT::Subjects::BindingsOutput, "max declarations set to: {}", this->max_declarations_per_file);
}


string BindingsOutputModule::get_name() {
    return "BindingsOutputModule";
}

OutputCriteria & BindingsOutputModule::get_criteria() {
    return this->criteria;
}



Template wrapper_builder_template(R"(
namespace v8toolkit {

template<>
struct WrapperBuilder<{{long_name}}> {
    void operator()(v8toolkit::Isolate & isolate)
        {{<<!class>>}}
};

}
)");

Template class_template(R"({
    v8toolkit::V8ClassWrapper<{{long_name}}> & class_wrapper = isolate.wrap_class<{{long_name}}>();
    class_wrapper.set_class_name("{{js_name}}");
{{<<member_functions|!!
    class_wrapper.add_method("{{js_name}}", {{name}}, {{default_arg_tuple}});>>}}

{{<<call_operator|!!
    class_wrapper.make_callable<{{binding_parameters}}>(&{{name}});>>}}

{{<<static_functions|!!
    class_wrapper.add_static_method<{{binding_parameters}}>("{{js_name}}", &{{name}}, {{default_arg_tuple}});>>}}

{{<<data_members|!!
    class_wrapper.add_member{{read_only}}<{{member_pointer}}>("{{js_name}}");>>}}

{{<<enums|!!
    class_wrapper.add_enum("{{name}}", \{{{elements%, |!{"{{name}}", {{value}}\}}}\});>>}}

{{<<wrapper_extension_methods|!!
    {{method_name}}(class_wrapper);>>}}

{{<<custom_extensions|!!
    {{}}>>}}
    class_wrapper.set_parent_type<{{<<base_type_name>}}>();
    class_wrapper.set_compatible_types<{{<<derived_types%, |!{{<name>}}>}}>();
    class_wrapper.finalize(true);
    {{<constructor>}}
})");


Template file_template(R"(
{{!standard_includes}}
// includes
{{<includes|!!
#include {{<include>}}>}}
// /includes

// explicit instantiations
{{<explicit_instantiations|!!
template class v8toolkit::V8ClassWrapper<{{<long_name>}}>;>}}
{{<explicit_instantiations_for_const_types|!!
template class v8toolkit::V8ClassWrapper<{{<long_name>}} const>;>}}

// /explicit instantiations

{{<extern_templates|!!
extern template {{class_name}}>}}

{{<pimpl_classes|wrapper_builder}}

void v8toolkit_initialize_class_wrappers_{{next_file_number}}(v8toolkit::Isolate &); // may not exist -- that's ok
void v8toolkit_initialize_class_wrappers_{{file_number}}(v8toolkit::Isolate & isolate) {

{{classes|!{{}}}}

{{call_next_function}}

}
)");


Template standard_includes_template(R"(
#include "js_casts.h"
#include <v8toolkit/v8_class_wrapper_impl.h>

)");


std::map<string, Template> bindings_templates {
    std::pair("class", class_template),
    std::pair("file", file_template),
    std::pair("standard_includes", standard_includes_template),
    std::pair("wrapper_builder", wrapper_builder_template)
};


} // end namespace v8toolkit::class_parser::bindings_output