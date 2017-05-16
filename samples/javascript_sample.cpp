#include <stdio.h>
#include <vector>
#include <map>
#include <string>

#include <fmt/format.h>

#include "javascript.h"

using namespace std;
using namespace v8toolkit;


int x = 1;
int y = 2;
int y2 = 3;


struct Thing {
    static void name(){}


    // must be careful because CastToNative
    Thing(vector<Thing>&&){}
};


auto run_static_function_tests() {
    Platform::set_max_memory(2000);
    auto i = Platform::create_isolate();
    ISOLATE_SCOPED_RUN(*i);
    V8ClassWrapper<Thing> & thing = i->wrap_class<Thing>();
    thing.add_static_method("get_name", &Thing::name);
    thing.finalize();
    thing.add_constructor<vector<Thing>&&>("Thing", *i);

    ContextPtr context = i->create_context();
    auto result = context->run("Thing.get_name();");

}

auto run_tests()
{
    
    auto ih1 = Platform::create_isolate();
    auto ih2 = Platform::create_isolate();
    
    return (*ih1)([&](){
        ih1->add_print();
        ih1->add_assert();
        
        // expose global variable x as "x" in all contexts created from this isolate
        ih1->expose_variable("x", x);
        ih1->add_function("return_hi", [](){return "hi";});

        auto c = ih1->create_context();
        auto c2 = ih1->create_context();

//        c->run("5");
//        c->run("assert('x == 1');");
        c->run("assert_contents(return_hi(), 'hi')");
        
        ih1->expose_variable("y", y);
        ih1->add_function("return_bye", [](){return "bye";});
        
        printf("*** Expecting exception\n");
        bool got_expected_failure = false;
        try {
            c->run("println(y)");
        } catch(v8toolkit::V8Exception & e) {
            got_expected_failure = true;
            printf("Expected failure, as 'y' is not present in contexts created before y was added to isolate: %s\n", e.what());
        }
        assert(got_expected_failure);
        got_expected_failure = false;
        try {
            c->run("println(return_bye())");
        } catch(v8toolkit::V8Exception & e) {
            got_expected_failure = true;
            printf("Expected failure, as 'return_bye' is not present in contexts created before return_bye was added to isolate: %s\n", e.what());
        }
        assert(got_expected_failure);
        got_expected_failure = false;
        
        
        auto c3 = ih1->create_context();
        printf("But they do exist on c3, as it was created after they were added to the isolate helper\n");
        c3->run("println('y is: ', y)");
        c3->run("println('return_bye(): ', return_bye())");
        
        (*c)([&](){
            // however, you can add them to an individual context
            c->expose_variable("y", y2);
            c->add_function("return_bye", [](){return "goodbye";});
        });
        
        printf("Now slightly different versions of 'y' and 'return_bye' have been added to c\n");
        c->run("println('y is:', y)");
        c->run("println('return_bye():', return_bye())");
        
        printf("but they're still not on c2, which was made at the same time as c\n");
        try {
            c2->run("println(y)");
        } catch(v8toolkit::V8Exception & e) {
            got_expected_failure = true;
            printf("Expected failure, as 'y' is not present in contexts created before y was added to isolate: %s\n", e.what());
        }
        assert(got_expected_failure);
        got_expected_failure = false;
        
        try {
            c2->run("println(return_bye())");
        } catch(v8toolkit::V8Exception & e) {
            got_expected_failure = true;
            printf("Expected failure, as 'return_bye' is not present in contexts created before return_bye was added to isolate: %s\n", e.what());
        }
        assert(got_expected_failure);
        got_expected_failure = false;
        
        
        // returning context to demonstrate how having a context alive keeps an Isolate alive even though the
        //   direct shared_ptr to it is destroyed at the end of this function
        return c;
    });
}


auto test_lifetimes()
{
    std::shared_ptr<Script> s;
    {
        std::shared_ptr<Context> c;
        {
            auto i = Platform::create_isolate();
            c = i->create_context();
        }
        // c is keeping i alive
        printf("Nothing should have been destroyed yet\n");
        
        (*c)([&](){
            s = c->compile("5");
        });
    }    
    // s is keeping c alive which is keeping i alive
    printf("Nothing should have been destroyed yet right before leaving test_lifetimes\n");
    
    return s->run_async();
    // The std::future returned from the async contains a shared_ptr to the context
}

// A/B/C should have interesting base offsets, with neither A nor B starting at the same place as C
struct A { int i=1; }; 
struct B { int i=2; }; 
struct C : A, B { int i=3; virtual ~C(){}};

class NotFamily {};

class NotWrapped {};

void run_type_conversion_test()
{
    auto i = Platform::create_isolate();
    (*i)([&]{
        auto & c_wrapper = i->wrap_class<C>();
        c_wrapper.finalize();
        c_wrapper.add_constructor("C", i->get_object_template());

        auto & a_wrapper = i->wrap_class<A>();
        a_wrapper.set_compatible_types<C>();
        a_wrapper.finalize();
        a_wrapper.add_constructor("A", i->get_object_template());

        auto & b_wrapper = i->wrap_class<B>();
        b_wrapper.set_compatible_types<C>();
        b_wrapper.finalize();
        b_wrapper.add_constructor("B", i->get_object_template());

        auto & notfamily_wrapper = i->wrap_class<NotFamily>();
        notfamily_wrapper.finalize();
        notfamily_wrapper.add_constructor("NotFamily", i->get_object_template());
        i->add_function("a", [](A * a) {
            printf("In 'A' function, A::i = %d (1) &a=%p\n", a->i, a);
        });
        i->add_function("b", [](B * b) {
            printf("In 'B' function, B::i = %d (2) &b=%p\n", b->i, b);
        });
        i->add_function("c", [](C * c) {
            printf("In 'C' function, C::i = %d (3) &c=%p\n", c->i, c);
        });
        i->add_function("not_wrapped", [](NotWrapped * nw) {
            printf("In 'not_wrapped' function\n");
        });
        auto c = i->create_context();
        c->run("a(new A())");
        c->run("a(new C())");
        c->run("b(new B())");
        c->run("b(new C())");
        c->run("c(new C())");
        
        printf("The following 3 lines are the same c++ object but casted to each type's starting address\n");
        c->run("var cvar = new C(); a(cvar); b(cvar); c(cvar);");
        
        
        try {
            c->run("a(new NotFamily())");
            printf("(BAD) Didn't catch exception\n");
        } catch(...) {
            printf("(GOOD) Caught exception calling parent() function with incompatible wrapped object\n");
        }
        try {
            c->run("not_wrapped(new C())");
            printf("(BAD) Didn't catch exception calling not_wrapped()\n");
        } catch(...) {
            printf("(GOOD) Caught exception calling not_wrapped when its parameter type is unknown to v8classwrapper\n");
        }
    });
}

void run_comparison_tests()
{
    auto i = Platform::create_isolate();
    i->add_assert();
    auto c = i->create_context();
    
    c->run("assert_contents(true, true);");
    bool got_expected_catch = false;
    try{
        c->run("assert_contents(true, false);");
    } catch (...) {
        got_expected_catch = true;
    }
    if (!got_expected_catch) {
        assert(false);
    }
    
    got_expected_catch = false;
    c->run("assert_contents(1,1);");
    try{
        c->run("assert_contents(1, 2);");
    } catch (...) {
        got_expected_catch = true;
    }
    if (!got_expected_catch) {
        assert(false);
    }
    
    
    got_expected_catch = false;
    c->run("assert_contents('asdf', 'asdf');");
    
    try{
        c->run("assert_contents('true', 'false');");
    } catch (...) {
        got_expected_catch = true;
    }
    if (!got_expected_catch) {
        assert(false);
    }
    
    
    got_expected_catch = false;
    c->run("assert_contents([1, 2, 3], [1, 2, 3]);");
    
    try{
        c->run("assert_contents([1, 2, 3], [3, 2, 1]);");
    } catch (...) {
        got_expected_catch = true;
    }
    if (!got_expected_catch) {
        assert(false);
    }
    
    got_expected_catch = false;    
    try{
        c->run("assert_contents([1, 2, 3], [1, 2, 3, 4]);");
    } catch (...) {
        got_expected_catch = true;
    }
    if (!got_expected_catch) {
        assert(false);
    }
    
    
    got_expected_catch = false;
    // these should be the same because order shouldn't matter
    c->run("assert_contents({'a': 4, 'b': 3}, {'b': 3, 'a': 4});");
    
    try{
        c->run("assert_contents({'a': 3, 'b': 3}, {'b': 3, 'a': 4});");
    } catch (...) {
        got_expected_catch = true;
    }
    if (!got_expected_catch) {
        assert(false);
    }
    
    
    got_expected_catch = false;
    c->run("assert_contents({'a': [1, 2, 3], 'b': [4, 5]}, {'b': [4, 5], 'a': [1, 2, 3]});");
    
    try{
        c->run("assert_contents({'a': [1, 2, true], 'b': [4, 5]}, {'b': [4, 5, {'c':9}], 'a': [1, 2, true]});");
    } catch (...) {
        got_expected_catch = true;
    }
    if (!got_expected_catch) {
        assert(false);
    }
}


void test_casts()
{
    auto i = Platform::create_isolate();
    i->add_assert();
    auto c = i->create_context();

    (*c)([&]{
    
        try {
            auto isolate = i->get_isolate();
            auto context = c->get_context();
            // printf("***** Testing STL container casts\n");

            std::vector<std::string> v{"hello", "there", "this", "is", "a", "vector"};
            c->add_variable("v", CastToJS<decltype(v)>()(isolate, v));
            c->run("assert_contents(v, ['hello', 'there', 'this', 'is', 'a', 'vector'])");

            std::list<float> l{1.5, 2.5, 3.5, 4.5};
            c->add_variable("l", CastToJS<decltype(l)>()(isolate, l));
            c->run("assert_contents(l, [1.5, 2.5, 3.5, 4.5]);");

            std::map<std::string, int> m{{"one", 1},{"two", 2},{"three", 3}};
            c->add_variable("m", CastToJS<decltype(m)>()(isolate, m));
            c->run("assert_contents(m, {'one': 1, 'two': 2, 'three': 3});");

            std::map<std::string, int> m2{{"four", 4},{"five", 5},{"six", 6}};
            c->add_variable("m2", CastToJS<decltype(m2)>()(isolate, m2));
            c->run("assert_contents(m2, {'four': 4, 'five': 5, 'six': 6});");

            std::deque<long> d{7000000000, 8000000000, 9000000000};
            add_variable(context, context->Global(), "d", CastToJS<decltype(d)>()(isolate, d));
            c->run("assert_contents(d, [7000000000, 8000000000, 9000000000]);");

            std::multimap<string, int> mm{{"a",1},{"a",2},{"a",3},{"b",4},{"c",5},{"c",6}};
            add_variable(context, context->Global(), "mm", CastToJS<decltype(mm)>()(isolate, mm));
            c->run("assert_contents(mm, {a: [1, 2, 3], b: [4], c: [5, 6]});");
            auto js_mm = c->run("mm");
            auto reconstituted_mm = CastToNative<decltype(mm)>()(isolate, js_mm.Get(isolate));
            assert(reconstituted_mm.size() == 6);
            assert(reconstituted_mm.count("a") == 3);
            assert(reconstituted_mm.count("b") == 1);
            assert(reconstituted_mm.count("c") == 2);

            std::array<int, 3> a{{1,2,3}};
            add_variable(context, context->Global(), "a", CastToJS<decltype(a)>()(isolate, a));
            c->run("assert_contents(a, [1, 2, 3]);");

            std::map<std::string, std::vector<int>> composite = {{"a",{1,2,3}},{"b",{4,5,6}},{"c",{7,8,9}}};
            add_variable(context, context->Global(), "composite", CastToJS<decltype(composite)>()(isolate, composite));
            c->run("assert_contents(composite, {'a': [1, 2, 3], 'b': [4, 5, 6], 'c': [7, 8, 9]});");

            {
                std::string tuple_string("Hello");
                auto tuple = make_tuple(1, 2.2, tuple_string);
                c->expose_variable("tuple", tuple);
                c->run("assert_contents(tuple, [1, 2.2, 'Hello'])");
            }
            // printf("Done testing STL container casts\n");


            auto unique = std::make_unique<std::vector<int>>(4, 1);
            // no support for read/write on unique_ptr
            c->expose_variable_readonly("unique", unique);
            c->run("assert_contents(unique, [1,1,1,1])");
            
            auto ulist = std::make_unique<std::list<int>>(4, 4);
            // no support for read/write on unique_ptr
            c->expose_variable_readonly("ulist", ulist);
            c->run("assert_contents(ulist, [4, 4, 4, 4])");
            
            
            auto shared = std::make_shared<std::vector<int>>(4, 3);
            // no support for read/write on unique_ptr
            c->expose_variable_readonly("shared", shared);
            c->run("assert_contents(shared, [3, 3, 3, 3])");
            
            std::vector<int> vector(4, 2);
            auto & vector_reference = vector;
            c->expose_variable("vector_reference", vector_reference);
            c->run("assert_contents(vector_reference, [2,2,2,2])");
            
            // this calls a function and tests that the function echo'd the input
            c->add_function("call_function", [](std::function<std::string(std::string)> function){
                auto result = function("hello");
                assert (result == "hello");
            });
            c->run("call_function(function(s){return s;})");

            // non-const tuple
            {
                auto js_tuple = c->run("[1.1, \"a\", [1, 2, 3], true]");
                auto tuple = CastToNative<std::tuple<double, std::string, std::vector<int>, bool>>()(*i, js_tuple.Get(*i));
                assert(std::get<0>(tuple) > 1.0 && std::get<0>(tuple) < 1.2);
                assert(std::get<1>(tuple) == "a");
                assert(std::get<2>(tuple).size() == 3);
                assert(std::get<2>(tuple)[0] == 1);
                assert(std::get<2>(tuple)[1] == 2);
                assert(std::get<2>(tuple)[2] == 3);
                assert(std::get<3>(tuple) == true);
            }

            // const tuple
            {
                auto js_tuple = c->run("[1.1, \"a\", [1, 2, 3], true]");
                auto tuple = CastToNative<std::tuple<double, std::string, std::vector<int>, bool> const>()(*i, js_tuple.Get(*i));
                assert(std::get<0>(tuple) > 1.0 && std::get<0>(tuple) < 1.2);
                assert(std::get<1>(tuple) == "a");
                assert(std::get<2>(tuple).size() == 3);
                assert(std::get<2>(tuple)[0] == 1);
                assert(std::get<2>(tuple)[1] == 2);
                assert(std::get<2>(tuple)[2] == 3);
                assert(std::get<3>(tuple) == true);
            }
            // pair
            {
                auto js_pair = c->run("[1, \"a\"]");
                auto pair = CastToNative<std::pair<int, std::string>>()(*i, js_pair.Get(*i));
                assert(pair.first == 1);
                assert(pair.second == "a");
            }
            // const pair
            {
                auto js_pair = c->run("[1, \"a\"]");
                auto pair = CastToNative<std::pair<int, std::string> const>()(*i, js_pair.Get(*i));
                assert(pair.first == 1);
                assert(pair.second == "a");
            }

            // vector
            {
                auto js_vector = c->run("[1, 2, 3]");
                auto vector = CastToNative<std::vector<int>>()(*i, js_vector.Get(*i));
                assert(vector.size() == 3);
                assert(vector[2] == 3);
            }
            // const vector
            {
                auto js_vector = c->run("[1, 2, 3]");
                auto vector = CastToNative<std::vector<int> const>()(*i, js_vector.Get(*i));
                assert(vector.size() == 3);
                assert(vector[2] == 3);
            }

            // map
            {
                auto js_map = c->run("new Object({a: 1, b: 2, c: 3})");
                auto map = CastToNative<std::map<std::string, int>>()(*i, js_map.Get(*i));
                assert(map.size() == 3);
                assert(map["c"] == 3);
            }
            // const map
            {
                auto js_map = c->run("new Object({a: 1, b: 2, c: 3})");
                auto map = CastToNative<std::map<std::string, int> const>()(*i, js_map.Get(*i));
                assert(map.size() == 3);
                assert(map["c"] == 3);
            }



        } catch (std::exception & e) {
            printf("Cast tests unexpectedily failed: %s\n", e.what());
            assert(false);  
        }
    });
}


void test_asserts()
{
    auto i = Platform::create_isolate();
    i->add_assert();
    i->add_print();
    auto c = i->create_context();
    
    bool caught_expected_assertion = false;
    try {
        c->run("assert('false')");
    } catch (...) {
        caught_expected_assertion = true;
    }
    assert(caught_expected_assertion);
    caught_expected_assertion = false;
    
    try {
        c->run("assert('0')");
    } catch (...) {
        caught_expected_assertion = true;
    }
    assert(caught_expected_assertion);
    caught_expected_assertion = false;
    
    try {
        c->run("assert(\"''\")");
    } catch (...) {
        caught_expected_assertion = true;
    }
    assert(caught_expected_assertion);
    caught_expected_assertion = false;
    
    try {
        c->run("assert('null')");
    } catch (...) {
        caught_expected_assertion = true;
    }
    assert(caught_expected_assertion);
    caught_expected_assertion = false;
    
    try {
        c->run("assert('undefined')");
    } catch (...) {
        caught_expected_assertion = true;
    }
    assert(caught_expected_assertion);
    caught_expected_assertion = false;
    
    try {
        c->run("assert('NaN')");
    } catch (...) {
        caught_expected_assertion = true;
    }
    assert(caught_expected_assertion);
    caught_expected_assertion = false;
    
    c->run("assert('true')");
    c->run("assert('1==1')");
    c->run("assert(\"'hello'\")");
    c->run("assert('[]')");
    c->run("assert('2')");
    c->run("assert('4.4')");
    c->run("if({}){println('{} is true');} else{println('{} is false');}");
    c->run("assert('({})')"); // the program "{}" is an empty program, not an empty object

    printf("Done testing asserts\n");
}



void require_directory_test()
{
    printf("In require directory test\n");
    auto i = Platform::create_isolate();
    i->add_assert();
    (*i)([&](){
        i->add_require();
        i->add_print();
        add_module_list(*i, i->get_object_template());
        auto c = i->create_context();
        (*c)([&]{
            require_directory(*c, "modules");
            c->run("printobj(module_list());");
            c->run("assert_contents(module_list()['modules/b.json'], {\"aa\": [1,2,3,{\"a\":4, \"b\": 5}, 6,7,8]})");
            c->run("printobj(module_list())");
            
            require_directory(*c, "modules");
            c->run("assert_contents(module_list()['modules/b.json'], {\"aa\": [1,2,3,{\"a\":4, \"b\": 5}, 6,7,8]})");
            c->run("printobj(module_list())");
            
        });
    });
}


struct IT_A {
    int get_int(){return 5;}
};

struct IT_B : public IT_A {

};

void run_inheritance_test()
{
    auto i = Platform::create_isolate();
    (*i)([&]{
        i->add_print();
        i->add_assert();
        
        // it's critical to wrap both classes and have the base class set the child as "compatible" and the child set the parent as "parent"
        auto & ita_wrapper = i->wrap_class<IT_A>();
        ita_wrapper.add_method("get_int", &IT_A::get_int);
        ita_wrapper.set_compatible_types<IT_B>();
        ita_wrapper.finalize();
        ita_wrapper.add_constructor<>("IT_A", *i);

        auto & itb_wrapper = i->wrap_class<IT_B>();
        itb_wrapper.set_parent_type<IT_A>();
        itb_wrapper.finalize();
        itb_wrapper.add_constructor<>("IT_B", *i);
        
        auto c = i->create_context();

        c->run("var it_b = new IT_B(); assert('it_b.get_int() == 5');");
        c->run("assert('Object.create(new IT_A()).get_int() == 5')");
        
        (*c)([&]{
            auto json = c->json("[1,2,3]");
            (void)get_value_as<v8::Array>(json);
            bool got_expected_exception = false;
            try{
                (void)get_value_as<v8::Boolean>(json);
            } catch(...) {
                got_expected_exception = true;
            }
            assert(got_expected_exception);
            got_expected_exception = false;
            printf("Done\n");
        });
    });
}

int rvalue_test_class_destructor_ran = 0;
class RvalueTestClass;
std::set<RvalueTestClass *> undeallocated_objects;
class RvalueTestClass {
public:
    RvalueTestClass(std::string const & str):str(str){
        undeallocated_objects.insert(this);
    }
    ~RvalueTestClass(){
//        std::cerr << fmt::format("Destroying {}", (void*)this) << std::endl;
        assert(undeallocated_objects.find(this) != undeallocated_objects.end());
        undeallocated_objects.erase(this);
        assert(std::atoi(str.c_str()) < 20000);rvalue_test_class_destructor_ran++;}
    RvalueTestClass(RvalueTestClass &&) = default;
    std::string str;
};

void takes_rvalue_ref(RvalueTestClass && rvalue_test_class) {
    RvalueTestClass moved_into(std::move(rvalue_test_class));
    undeallocated_objects.insert(&moved_into); // normal constructor not run, so it wouldn't otherwise be inserted causing assertion to fire in destructor
    std::cerr << fmt::format("moved_into: {}", moved_into.str) << std::endl;
}

void takes_unique_ptr(std::unique_ptr<RvalueTestClass> rvalue_test_class) {
    int c1 = undeallocated_objects.size();
//    std::cerr << fmt::format("in takes unique ptr with {}", (void*)rvalue_test_class.get()) << std::endl;
    rvalue_test_class.reset();
    assert(undeallocated_objects.size() == c1 - 1);
}



void test_rvalues() {
    auto isolate = Platform::create_isolate();
    ISOLATE_SCOPED_RUN(isolate->get_isolate());
    isolate->add_function("takes_rvalue_ref", &takes_rvalue_ref);
    isolate->add_function("takes_unique_ptr", &takes_unique_ptr);
    auto & wrapper = isolate->wrap_class<RvalueTestClass>();
    wrapper.finalize();
    wrapper.add_constructor<char *>("RvalueTestClass", *isolate);
    auto c = isolate->create_context();
    auto moved_out_of_str = c->run("let str = new RvalueTestClass(\"asdf\");"
        "takes_rvalue_ref(str);"
        "str;");

    std::cerr << fmt::format("remaining: {}", CastToNative<RvalueTestClass>()(*isolate, moved_out_of_str.Get(*isolate)).str) << std::endl;


    // test a little GC..
    rvalue_test_class_destructor_ran = 0;

    c->run("for(let i = 0; i < 20000; i++) {new RvalueTestClass(`${i}`);}");
//    c->run("for(let i = 0; i < 20000; i++){new RvalueTestClass(\"some random string\");}");
    while(!isolate->get_isolate()->IdleNotification(1000)){}
    std::cerr << fmt::format("destructor ran {} times", rvalue_test_class_destructor_ran) << std::endl;
    assert(rvalue_test_class_destructor_ran > 100); // if this fires, increase the test count in the javascript above

    // CastToNative<unique_ptr> should clear the GC callback to free the RvalueTestClass object for these calls
    c->run("for(let i = 20000; i < 30000; i++) {takes_unique_ptr(new RvalueTestClass(\"{}\"));}");

    // original object from test of moving it may still exist, but that should be the only one
    while(!isolate->get_isolate()->IdleNotification(1000)){}
    std::cerr << fmt::format("destructor ran {} times (should be 30000) with {} remaining objects", rvalue_test_class_destructor_ran, undeallocated_objects.size()) << std::endl;
    assert(rvalue_test_class_destructor_ran == 30000);
    assert(undeallocated_objects.size() < 2);


}

class TestClass{
public:
    int i;
    void func(){};
    TestClass(char*){}
};

int main(int argc, char ** argv) {
    
    Platform::init(argc, argv, argv[0]);
    require_directory_test();


    run_type_conversion_test();

    auto future = test_lifetimes();
    printf("Nothing should have been destroyed yet\n");
    {
        auto results = future.get();
        results.first.Reset();
        printf("Nothing should have been destroyed yet after getting future results\n");
    }
    printf("The script, context, and isolate helpers should have all been destroyed\n");

    run_static_function_tests();

    auto context = run_tests();
    printf("The script, context, and isolate helpers should have all been destroyed\n");

    printf("after run_tests, one isolate helper was destroyed, since it made no contexts\n");

    printf("Running comparison tests\n");
    run_comparison_tests();

    printf("Testing casts\n");
    test_casts();

    printf("Testing asserts\n");
    test_asserts();
    
    std::cerr << fmt::format("Testing rvalues") << std::endl;
    test_rvalues();


    run_inheritance_test();

    printf("Program ending, so last context and the isolate that made it will now be destroyed\n");


    // try to make a duplicate isolate
    std::cerr << fmt::format("Testing for proper handling of a subsequent isolate with the same address as a previous one") << std::endl;
    std::set<v8::Isolate *> isolate_addresses;
    bool got_duplicate = false;
    for(int i = 0; i < 100; i++) {
        auto isolate = Platform::create_isolate();
        ISOLATE_SCOPED_RUN(isolate->get_isolate());
//        std::cerr << fmt::format("Created isolate at {}", (void*)*isolate) << std::endl;
        V8ClassWrapper<TestClass> & wrapper = isolate->wrap_class<TestClass>();
        wrapper.add_member<int, TestClass, &TestClass::i>("i");
        wrapper.add_method("func", &TestClass::func);
        wrapper.finalize();
        wrapper.add_constructor<char*>("TestClass", *isolate);

        if (isolate_addresses.find(isolate->get_isolate()) != isolate_addresses.end()) {
            std::cerr << fmt::format("found duplicate, done looking") << std::endl;
            got_duplicate = true;
            break;
        }
        isolate_addresses.insert(isolate.get()->get_isolate());
    }
    if (!got_duplicate) {
        std::cerr << fmt::format("duplicate isolate address not received, so couldn't test v8classwrapper cleanup") << std::endl;
    } else {
        std::cerr << fmt::format("Successfully recreated bindings on a subsequent isolate with the same address as a previous one") << std::endl;
    }
}

