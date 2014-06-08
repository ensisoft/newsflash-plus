// Copyright (c) 2014 Sami Väisänen, Ensisoft 
//
// http://www.ensisoft.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.

//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.            

#pragma once

#include <typeinfo>
#include <cstddef>
#include <memory>
#include <map>

namespace sdk
{
    class message_register
    {
    public:
        class type 
        {
        public:
            virtual ~type() = default;

            template<typename T>
            bool match() const 
            {
                const std::type_info& other = typeid(T);
                return is_match(other);
            }
            virtual const char* name() const = 0;

        protected:
            virtual bool is_match(const std::type_info& other) const = 0;
        };

        // two way lookup, lookup the type based on the id
        // and lookup the id given a type

        static 
        const type* find(std::size_t id)
        {
            const auto& map = types();
            const auto& it = map.find(id);
            if (it == map.end())
                return nullptr;

            const auto* ptr = it->second.get();
            return ptr;
        }

        template<typename T> static
        std::size_t find()
        {
            T force_instanciation;

            const auto& map = types();

            // linear search..
            for (auto& it : map)
            {
                const auto& type = it.second;
                if (type->match<T>())
                    return it.first;
            }
            return 0;
        }

    private:
        template<typename T>
        class any_type : public type 
        {
        protected:
            virtual const char* name() const override
            {
                return typeid(T).name();
            }

            virtual bool is_match(const std::type_info& other) const override 
            {
                return typeid(T) == other;
            }
        };

        typedef 
        std::map<std::size_t, std::shared_ptr<type>> map_t;

        static
        map_t& types();

        static
        std::size_t get_next_command_id();

    public:
        template<typename T>
        class registrant 
        {
            typedef any_type<T> my_type;
        public:
            registrant() : id_(get_next_command_id()) 
            {
                auto& map = types();
                map.insert({id_, std::make_shared<my_type>()});
            }
            std::size_t identity() const 
            {
                return id_;
            }
        private:
            const std::size_t id_;
        };        
    };

} // sdk
