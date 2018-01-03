#include "Util.hpp"

#include <assert.h>
#include <iostream>
#include <sstream>

static std::ostream &PrintObject(std::ostream &os, const std::shared_ptr<const lisp_object> obj) {
    if(obj->meta() && obj->meta()->count() > 0) {
      PrintObject(os << "#^", obj->meta()) << " ";
    }

    if(obj == List::Empty) {
        os << "()";
        return os;
    }
  
    std::shared_ptr<const ISeq> list = std::dynamic_pointer_cast<const ISeq>(obj);
    if(list) {
        PrintObject(os << "(", list->first());
        for(list = list->next(); list != NULL; list = list->next())
            PrintObject(os << " ", list->first());
        os << ")";
        return os;
    }
  
    std::shared_ptr<const IMap> map = std::dynamic_pointer_cast<const IMap>(obj);
    if(map) {
        os << "{";
        for(list = map->seq(); list != NULL; list = list->next()) {
            std::shared_ptr<const MapEntry> me = std::dynamic_pointer_cast<const MapEntry>(list->first());
            assert(me != NULL);
            PrintObject(os, me->key);
            PrintObject(os << " ", me->val);
            if(list->next() != NULL) os << ", ";
        }
        return os << "}";
    }

    std::shared_ptr<const IVector> vec = std::dynamic_pointer_cast<const IVector>(obj);
    if(vec) {
        os << "[";
        if(vec->count() > 0)
            PrintObject(os, vec->nth(0));
        for(size_t i = 1; i < vec->count(); i++)
            PrintObject(os << " ", vec->nth(i));
        return os << "]";
    }
  
    return os << obj->toString();
}

std::string toString(const std::shared_ptr<const lisp_object> o) {
    std::stringstream buffer;
    PrintObject(buffer, o);
    return buffer.str();
}
