// one file for N classes

#pragma once

#include <vector>
#include <fstream>
#include <iostream>


#include "../output_modules.h"


namespace v8toolkit::class_parser {


// returns whether a WrappedClass object should be part of the JavaScript stub
class BidirectionalCriteria : public OutputCriteria {
    bool operator()(WrappedClass const & c);
};



class BidirectionalOutputStreamProvider : public OutputStreamProvider {
private:
//    std::stringstream string_stream;
    std::ofstream output_file;
public:
    std::ostream & get_class_collection_stream() override;

    ostream & get_class_stream(WrappedClass const & c) override;


    ~BidirectionalOutputStreamProvider()
    {}
};



class BidirectionalOutputModule : public OutputModule {
private:
    BidirectionalCriteria criteria;
public:

    std::stringstream string_stream;
    BidirectionalOutputModule() : OutputModule(std::make_unique<StringStreamOutputStreamProvider>(string_stream))
    {}

    void process(std::vector<WrappedClass const *> wrapped_classes) override;

    string get_name() override;

    OutputCriteria & get_criteria() override;
};


} // end namespace v8toolkit::class_parser