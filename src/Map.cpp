#include <iostream>

#include <cassert>
#include <map>
#include <memory>
#include <string>
#include <functional>

class ArityException : std::runtime_error {
  public:
    const size_t actual;
    const std::string name;
    
    ArityException(size_t actual, std::string name) : actual(actual), name(name), std::runtime_error("Wrong number of args (" + std::to_string(actual) + ") passed to: " + name) {};
};

class IMap;
class ISeq;

class lisp_object : public std::enable_shared_from_this<lisp_object> {
  public:
    virtual std::string toString(void) {return ((lisp_object * const) this)->toString();};
    virtual std::string toString(void) const = 0;
    virtual bool equals(std::shared_ptr<const lisp_object> o) const {return o.get() == this;};
    bool operator==(std::shared_ptr<const lisp_object> o) const {return equals(o);};
    lisp_object& operator=(lisp_object&&) = delete;
    virtual size_t hashCode() const {return std::hash<const lisp_object*>()(this);};
};

class IMapEntry {
  public:
    virtual std::shared_ptr<const lisp_object> key() const = 0;
    virtual std::shared_ptr<const lisp_object> val() const = 0;
};
class IMeta : public virtual lisp_object {
  public:
    virtual std::shared_ptr<const IMeta> with_meta(std::shared_ptr<const IMap>) const = 0;
    std::shared_ptr<const IMap> meta(void) const {return _meta;};
  protected:
    std::shared_ptr<const IMap> _meta;
    IMeta(void) : _meta(NULL) {};
    IMeta(std::shared_ptr<const IMap> meta) : _meta(meta) {};
};
class IFn : virtual public lisp_object {
  public:
    virtual std::shared_ptr<const lisp_object> invoke() const = 0;
    virtual std::shared_ptr<const lisp_object> invoke(std::shared_ptr<const lisp_object>) const = 0;
    virtual std::shared_ptr<const lisp_object> invoke(std::shared_ptr<const lisp_object>, std::shared_ptr<const lisp_object>) const = 0;
    virtual std::shared_ptr<const lisp_object> invoke(std::shared_ptr<const lisp_object>, std::shared_ptr<const lisp_object>, std::shared_ptr<const lisp_object>) const = 0;
    virtual std::shared_ptr<const lisp_object> invoke(std::shared_ptr<const lisp_object>, std::shared_ptr<const lisp_object>, std::shared_ptr<const lisp_object>, std::shared_ptr<const lisp_object>) const = 0;
    virtual std::shared_ptr<const lisp_object> applyTo(std::shared_ptr<const ISeq>) const = 0;
};
class IReference : public IMeta {
  public:
    // virtual std::shared_ptr<const IMap> alterMeta(IFn alter, ISeq args) = 0;
    virtual std::shared_ptr<const IMap> resetMeta(std::shared_ptr<const IMap> m) = 0;
  protected:
    IReference() {};
    IReference(std::shared_ptr<const IMap> meta) : IMeta(meta) {};
};
class Named {
  public:
    virtual std::string getName() const = 0;
    virtual std::string getNamespace() const = 0;
};
class Comparable {
  public:
    virtual int compare(std::shared_ptr<const lisp_object> o) const = 0;
    bool operator< (std::shared_ptr<const lisp_object> o) {return compare(o) <  0;};
    bool operator<=(std::shared_ptr<const lisp_object> o) {return compare(o) <= 0;};
    bool operator> (std::shared_ptr<const lisp_object> o) {return compare(o) >  0;};
    bool operator>=(std::shared_ptr<const lisp_object> o) {return compare(o) >= 0;};
};
class ILookup : virtual public lisp_object {
  public:
    // virtual std::shared_ptr<const lisp_object> valAt(std::shared_ptr<const lisp_object> key) const = 0;
    // virtual std::shared_ptr<const lisp_object> valAt(std::shared_ptr<const lisp_object> key,
    //                                                  std::shared_ptr<const lisp_object> NotFound) const = 0;
};
class Seqable : virtual public lisp_object {
  public :
    virtual std::shared_ptr<const ISeq> seq() const = 0;
};
class Counted : virtual public lisp_object {
  public:
    virtual size_t count(void) const = 0;
};
class ICollection : virtual public Seqable, virtual public Counted {
  public:
    virtual size_t count(void) const = 0;
    // virtual std::shared_ptr<const ICollection>cons(std::shared_ptr<const ISeq>, std::shared_ptr<const lisp_object>)const = 0;
    virtual std::shared_ptr<const ICollection>empty()const = 0;
    virtual bool equiv(std::shared_ptr<const lisp_object>)const {return false;} /* TODO = 0 */;
};
class Associative2 {
  public:
    // virtual bool containsKey(std::shared_ptr<const lisp_object>) const = 0;
    // virtual std::shared_ptr<const lisp_object> entryAt(std::shared_ptr<const lisp_object>) const = 0;

};
class Associative : virtual public ICollection, virtual public ILookup, virtual public Associative2 {
  public:
    std::shared_ptr<const Associative> assoc(std::shared_ptr<const lisp_object> key,
                                             std::shared_ptr<const lisp_object> val) const {
      return std::shared_ptr<const Associative>(assoc_impl(key, val));
    };
  private:
    virtual const Associative *assoc_impl(std::shared_ptr<const lisp_object>,
                                          std::shared_ptr<const lisp_object>) const = 0;
};
template <class Derived, class ...Bases>
class Associative_inherit : virtual public Associative, public Bases... {
  public:
    std::shared_ptr<const Derived> assoc(std::shared_ptr<const lisp_object> key,
                                         std::shared_ptr<const lisp_object> val) const {
      return std::shared_ptr<const Derived>(static_cast<const Derived*>(assoc_impl(key, val)));
    };
  private:
    Associative_inherit() = default;
    friend Derived;
    virtual const Associative_inherit *assoc_impl(std::shared_ptr<const lisp_object>,
                                      std::shared_ptr<const lisp_object>) const = 0;
};
class IMap : public Associative_inherit<IMap> {
  public:
    // virtual std::shared_ptr<const IMap> assocEx(std::shared_ptr<const lisp_object>,
    //                                                  std::shared_ptr<const lisp_object>) const = 0;
    virtual std::shared_ptr<const IMap> without(std::shared_ptr<const lisp_object>) const = 0;
};
class ISeq : public ICollection {
  public:
    virtual std::shared_ptr<const lisp_object> first() const = 0;
    virtual std::shared_ptr<const ISeq> next() const = 0;
    virtual std::shared_ptr<const ISeq> more() const = 0;
};
class IDeref {
  public:
    virtual std::shared_ptr<const lisp_object> deref() = 0;
};
class IRef : public IDeref {
  public:
    virtual void setValidator(std::shared_ptr<const IFn>) = 0;
    virtual std::shared_ptr<const IFn> getValidator() = 0;
    virtual std::shared_ptr<const IMap> getWatches() = 0;
    virtual std::shared_ptr<IRef> addWatch(std::shared_ptr<const lisp_object>, std::shared_ptr<const IFn>) = 0;
    virtual std::shared_ptr<IRef> removeWatch(std::shared_ptr<const lisp_object>) = 0;
};
class Settable {
  public:
    virtual std::shared_ptr<const lisp_object> doSet(std::shared_ptr<const lisp_object>) const = 0;
    virtual std::shared_ptr<const lisp_object> doReset(std::shared_ptr<const lisp_object>) const = 0;
};

class ITransientCollection {
  public:
    virtual std::shared_ptr<ITransientCollection> conj(std::shared_ptr<const lisp_object>) = 0;
    virtual std::shared_ptr<const ICollection> persistent() = 0;
};
class IEditableCollection {
  public:
    virtual std::shared_ptr<ITransientCollection> asTransient() const = 0;
};

class ITransientAssociative : public virtual ITransientCollection, public virtual ILookup, public virtual Associative2 {
  public:
    std::shared_ptr<ITransientAssociative> assoc(std::shared_ptr<const lisp_object> key,
                                                 std::shared_ptr<const lisp_object> val) {
      return std::shared_ptr<ITransientAssociative>(static_cast<ITransientAssociative*>(assoc_impl(key, val)));
    };
  private:
    virtual ITransientAssociative *assoc_impl(std::shared_ptr<const lisp_object>,
                                              std::shared_ptr<const lisp_object>) = 0;

};
template <class Derived, class ...Bases>
class ITransientAssociative_inherit : virtual public ITransientAssociative, public Bases... {
  public:
    std::shared_ptr<Derived> assoc(std::shared_ptr<const lisp_object> key,
                                   std::shared_ptr<const lisp_object> val) {
      return std::shared_ptr<Derived>(static_cast<Derived*>(assoc_impl(key, val)));
    };
  private:
    ITransientAssociative_inherit() = default;
    friend Derived;
    virtual ITransientAssociative_inherit *assoc_impl(std::shared_ptr<const lisp_object>, std::shared_ptr<const lisp_object>) = 0;
};

class ITransientMap : public ITransientAssociative_inherit<ITransientMap>, public Counted {
  public:
    virtual std::shared_ptr<ITransientMap> without(std::shared_ptr<const lisp_object> key) = 0;
};

class Number : public lisp_object {};
// Util.cpp

std::string toString(std::shared_ptr<const lisp_object>) {
  return "";
}

std::shared_ptr<const lisp_object> ret1(std::shared_ptr<const lisp_object> ret, std::shared_ptr<const lisp_object>) {
  return ret;
}

std::shared_ptr<const ISeq> ret1(std::shared_ptr<const ISeq> ret, std::shared_ptr<const lisp_object>) {
  return ret;
}

bool equiv(std::shared_ptr<const lisp_object> x, std::shared_ptr<const lisp_object> y) {
  if(x == y)
    return true;
  if(x) {
    std::shared_ptr<const Number> x_num = std::dynamic_pointer_cast<const Number>(x);
    if(x_num) {
      std::shared_ptr<const Number> y_num = std::dynamic_pointer_cast<const Number>(y);
      if(y_num)
        return x_num == y_num;
    } else {
      std::shared_ptr<const ICollection> x_coll = std::dynamic_pointer_cast<const ICollection>(x);
      if(x_coll)
        return x_coll->equiv(y);
      std::shared_ptr<const ICollection> y_coll = std::dynamic_pointer_cast<const ICollection>(y);
      if(y_coll)
        return y_coll->equiv(x);
    }
    return x == y;
  }
  return false;
}

size_t hash(std::shared_ptr<const lisp_object> o) {
  if(o == NULL)
    return 0;
  return o->hashCode();
}

size_t hashEq(std::shared_ptr<const lisp_object> o) {
  // TODO
  return 0;
}

// AFn.cpp

class AFn : public IFn {
  public:
    std::shared_ptr<const lisp_object> throwArity(size_t n) const;
    virtual std::shared_ptr<const lisp_object> invoke() const {return throwArity(0);} ;
    virtual std::shared_ptr<const lisp_object> invoke(std::shared_ptr<const lisp_object>) const {return throwArity(1);} ;
    virtual std::shared_ptr<const lisp_object> invoke(std::shared_ptr<const lisp_object>, std::shared_ptr<const lisp_object>) const {return throwArity(2);} ;
    virtual std::shared_ptr<const lisp_object> invoke(std::shared_ptr<const lisp_object>, std::shared_ptr<const lisp_object>, std::shared_ptr<const lisp_object>) const {return throwArity(3);} ;
    virtual std::shared_ptr<const lisp_object> invoke(std::shared_ptr<const lisp_object>, std::shared_ptr<const lisp_object>, std::shared_ptr<const lisp_object>, std::shared_ptr<const lisp_object>) const {return throwArity(4);} ;
    virtual std::shared_ptr<const lisp_object> applyTo(std::shared_ptr<const ISeq>) const;
    virtual std::shared_ptr<const lisp_object> applyToHelper(std::shared_ptr<const IFn>, std::shared_ptr<const ISeq>) const;
};

std::shared_ptr<const lisp_object> AFn::throwArity(size_t n) const {
  throw ArityException(n, toString());
}

std::shared_ptr<const lisp_object> AFn::applyTo(std::shared_ptr<const ISeq> arglist) const {
  return applyToHelper(std::dynamic_pointer_cast<const IFn>(shared_from_this()), ret1(arglist, arglist = NULL));
}

std::shared_ptr<const lisp_object> AFn::applyToHelper(std::shared_ptr<const IFn>, std::shared_ptr<const ISeq>) const {
  // TODO
}


// Symbol.cpp

class Symbol : /* public AFn, */ public IMeta, public Named, public Comparable {
  public:
    virtual std::string toString(void) const;
    virtual std::string getName() const;
    virtual std::string getNamespace() const;
    virtual std::shared_ptr<const IMeta> with_meta(std::shared_ptr<const IMap>) const;
    virtual bool equals(std::shared_ptr<const lisp_object>) const;
    virtual int compare(std::shared_ptr<const lisp_object>) const;
    bool operator< (std::shared_ptr<const lisp_object> o) {return Comparable::operator<(o);};

    static std::shared_ptr<Symbol> intern(std::string nsname);
    static std::shared_ptr<Symbol> intern(std::string ns, std::string name);
  private:
    const std::string ns;
    const std::string name;
    
    Symbol(std::string ns, std::string name) : ns(ns), name(name) {};
    Symbol(std::shared_ptr<const IMap> meta, std::string ns, std::string name) : IMeta(meta), ns(ns), name(name) {};
};

std::string Symbol::toString(void) const {
  if(ns.size() == 0)
    return name;
  return ns + "/" + name;
}

std::string Symbol::getName() const {
  return name;
}

std::string Symbol::getNamespace() const {
  return ns;
}

std::shared_ptr<Symbol> Symbol::intern(std::string nsname) {
  size_t i = nsname.find('/');
  if(i == std::string::npos || nsname == "/")
    return std::shared_ptr<Symbol>(new Symbol("", nsname));
  return std::shared_ptr<Symbol>(new Symbol(nsname.substr(0,i), nsname.substr(i+1)));
}

std::shared_ptr<Symbol> Symbol::intern(std::string ns, std::string name) {
  return std::shared_ptr<Symbol>(new Symbol(ns, name));
}

std::shared_ptr<const IMeta> Symbol::with_meta(std::shared_ptr<const IMap> meta) const {
  return std::shared_ptr<Symbol>(new Symbol(meta, ns, name));
}

bool Symbol::equals(std::shared_ptr<const lisp_object> o) const {
  if(o.get() == this)
    return true;
  std::shared_ptr<const Symbol> s = std::dynamic_pointer_cast<const Symbol>(o);
  if(s)
    return (ns == s->ns) && (name == s->name);
  return false;
}

int Symbol::compare(std::shared_ptr<const lisp_object> o) const {
  if(equals(o))
    return 0;
  std::shared_ptr<const Symbol> s = std::dynamic_pointer_cast<const Symbol>(o);
  assert(s);
  if(ns.size() == 0 && s->ns.size() > 0)
    return -1;
  if(ns.size() > 0) {
    if(s->ns.size() > 0)
      return 1;
    int nsc = ns.compare(s->ns);
    if(nsc)
      return nsc;
  }
  return name.compare(s->name);
}


// Keyword.cpp

class Keyword : public Named, public Comparable /* IFn, IHashEq */ {
  public:
    virtual std::string toString(void) const;
    virtual std::string getName() const;
    virtual std::string getNamespace() const;
    virtual int compare(std::shared_ptr<const lisp_object> o) const;

    static std::shared_ptr<Keyword> intern(std::shared_ptr<const Symbol> sym);
    static std::shared_ptr<Keyword> intern(std::string nsname);
    static std::shared_ptr<Keyword> intern(std::string ns, std::string name);
    static std::shared_ptr<Keyword> find(std::shared_ptr<const Symbol> sym);
    static std::shared_ptr<Keyword> find(std::string nsname);
    static std::shared_ptr<Keyword> find(std::string ns, std::string name);

    const std::shared_ptr<const Symbol> sym;
  private:
    static std::map<std::shared_ptr<const Symbol>, std::weak_ptr<Keyword> > table;
    Keyword(std::shared_ptr<const Symbol> sym) : sym(sym) {};
};

std::string Keyword::toString(void) const {
  return ":" + sym->toString();
}

std::string Keyword::getName() const {
  return sym->getName();
}

std::string Keyword::getNamespace() const {
  return sym->getNamespace();
}

int Keyword::compare(std::shared_ptr<const lisp_object> o) const {
  std::shared_ptr<const Keyword> s = std::dynamic_pointer_cast<const Keyword>(o);
  assert(s);
  return sym->compare(s->sym);
}

std::map<std::shared_ptr<const Symbol>, std::weak_ptr<Keyword> > Keyword::table = {};

std::shared_ptr<Keyword> Keyword::intern(std::shared_ptr<const Symbol> sym) {
  try {
    return table.at(sym).lock();
  } catch(std::out_of_range) {
    if(sym->meta())
      sym = std::dynamic_pointer_cast<const Symbol>(sym->with_meta(NULL));
    std::shared_ptr<Keyword> k = std::shared_ptr<Keyword>(new Keyword(sym));
    table[sym] = k;
    return k;
  }
}

std::shared_ptr<Keyword> Keyword::intern(std::string nsname) {
  return intern(Symbol::intern(nsname));
}

std::shared_ptr<Keyword> Keyword::intern(std::string ns, std::string name) {
  return intern(Symbol::intern(ns, name));
}

std::shared_ptr<Keyword> Keyword::find(std::shared_ptr<const Symbol> sym) {
  try {
    table.at(sym).lock();
  } catch(std::out_of_range) {
    return NULL;
  }
}

std::shared_ptr<Keyword> Keyword::find(std::string nsname) {
  return find(Symbol::intern(nsname));
}

std::shared_ptr<Keyword> Keyword::find(std::string ns, std::string name) {
  return find(Symbol::intern(ns, name));
}


// AMap.cpp

class AMap : public AFn, public IMap {
  public:
    virtual std::string toString(void) const {return ::toString(shared_from_this());};
};

class ATransientMap : /* public AFn, */public ITransientMap {
};


// Map.hpp
#include <thread>
class INode;
class TransientMap;

class LMap : public Associative_inherit<LMap, AMap>, public IEditableCollection, public IMeta /* IMapIterable, IKVReduce */ {
  public:
    virtual size_t count() const;
    virtual std::shared_ptr<const ISeq> seq() const;
    virtual std::shared_ptr<const ICollection> empty() const;
    virtual std::shared_ptr<const IMap> without(std::shared_ptr<const lisp_object>) const;
    virtual std::shared_ptr<const IMeta> with_meta(std::shared_ptr<const IMap>) const;
    virtual std::shared_ptr<ITransientCollection> asTransient() const;

    static std::shared_ptr<const LMap> EMPTY;
  private:
    const size_t _count;
    const std::shared_ptr<const INode> root;
    const bool hasNull;
    const std::shared_ptr<const lisp_object> nullValue;

    LMap(std::shared_ptr<const IMap> meta, size_t count, std::shared_ptr<const INode> root, bool hasNull, std::shared_ptr<const lisp_object> nullValue) : IMeta(meta), _count(count), root(root), hasNull(hasNull), nullValue(nullValue) {};
    LMap(size_t count, std::shared_ptr<const INode> root, bool hasNull, std::shared_ptr<const lisp_object> nullValue) : _count(count), root(root), hasNull(hasNull), nullValue(nullValue) {};
    virtual const LMap* assoc_impl(std::shared_ptr<const lisp_object>, std::shared_ptr<const lisp_object>) const;
    friend TransientMap;
};

class TransientMap : public ITransientAssociative_inherit<TransientMap, ATransientMap> {
  public:
    TransientMap(size_t hashId, size_t count, std::shared_ptr<INode> root, bool hasNull, std::shared_ptr<const lisp_object> nullValue) : hashId(hashId), _count(count), root(root), hasNull(hasNull), nullValue(nullValue) {};
    virtual std::string toString() const;
    virtual size_t count() const;
    virtual std::shared_ptr<ITransientCollection> conj(std::shared_ptr<const lisp_object>);
    virtual std::shared_ptr<const ICollection> persistent();
    virtual std::shared_ptr<ITransientMap> without(std::shared_ptr<const lisp_object> key);
  private:
    size_t hashId;
    size_t _count;
    std::shared_ptr<INode> root;
    bool hasNull;
    std::shared_ptr<const lisp_object> nullValue;

    virtual TransientMap *assoc_impl(std::shared_ptr<const lisp_object>, std::shared_ptr<const lisp_object>);
    void ensureEditable() const;
};


// AReference.hpp
class AReference : public IReference {
  public:
    virtual std::shared_ptr<const IMap> resetMeta(std::shared_ptr<const IMap> m) {IMeta::_meta = m; return m;};
  protected:
    AReference() {};
    AReference(std::shared_ptr<const IMap> meta) : IReference(meta) {};
};


// ARef.cpp
class ARef : public AReference, public IRef {
  public:
    ARef() : validator(NULL), watches(LMap::EMPTY) {};
    ARef(std::shared_ptr<const IMap> meta) : AReference(meta), validator(NULL), watches(LMap::EMPTY) {};

    virtual void setValidator(std::shared_ptr<const IFn>);
    virtual std::shared_ptr<const IFn> getValidator();
    virtual std::shared_ptr<const IMap> getWatches();
    virtual std::shared_ptr<IRef> addWatch(std::shared_ptr<const lisp_object>, std::shared_ptr<const IFn>);
    virtual std::shared_ptr<IRef> removeWatch(std::shared_ptr<const lisp_object>);
    void notifyWatches(std::shared_ptr<const lisp_object> oldval, std::shared_ptr<const lisp_object> newval);
protected:
    std::shared_ptr<const IFn> validator;
  private:
    std::shared_ptr<const IMap> watches;
    
    void validate(std::shared_ptr<const IFn>, std::shared_ptr<const lisp_object>);
};

void ARef::setValidator(std::shared_ptr<const IFn> f) {
  validate(f, deref());
  validator = f;
}

std::shared_ptr<const IFn> ARef::getValidator() {
  return validator;
}

std::shared_ptr<const IMap> ARef::getWatches() {
  return watches;
}

std::shared_ptr<IRef> ARef::addWatch(std::shared_ptr<const lisp_object> key, std::shared_ptr<const IFn> callback) {
  watches = std::dynamic_pointer_cast<const IMap>(watches->assoc(key, callback));
  return std::dynamic_pointer_cast<IRef>(shared_from_this());
}

std::shared_ptr<IRef> ARef::removeWatch(std::shared_ptr<const lisp_object> key) {
  watches = std::dynamic_pointer_cast<const IMap>(watches->without(key));
  return std::dynamic_pointer_cast<IRef>(shared_from_this());
}

void ARef::notifyWatches(std::shared_ptr<const lisp_object> oldval, std::shared_ptr<const lisp_object> newval) {
  std::shared_ptr<const IMap> ws = watches;
  if(ws->count() > 0) {
    for(std::shared_ptr<const ISeq> s = ws->seq(); s != NULL; s = s->next()) {
      std::shared_ptr<const IMapEntry> e = std::dynamic_pointer_cast<const IMapEntry>(s->first());
      std::shared_ptr<const IFn> f = std::dynamic_pointer_cast<const IFn>(e->val());
      if(f)
        f->invoke(e->key(), shared_from_this(), oldval, newval);
    }
  }
}

void ARef::validate(std::shared_ptr<const IFn> f, std::shared_ptr<const lisp_object> val) {
  try {
    if(f && !(bool)f->invoke(val))
      throw std::runtime_error("Invalid reference state");
  } catch(std::runtime_error e) {
    throw e;
  } catch(std::exception &e) {
    throw std::runtime_error(std::string("Invalid reference state: ") + e.what());
  }
}


// Namespace.cpp
class Namespace : public AReference {
  public:
    const std::shared_ptr<const Symbol> name;
    
    virtual std::string toString(void) const;
    virtual std::shared_ptr<const IMeta> with_meta(std::shared_ptr<const IMap>) const;
    
    static std::shared_ptr<Namespace> findOrCreate(std::shared_ptr<const Symbol> name);
  private:
    std::shared_ptr<const IMap> mappings;
    std::shared_ptr<const IMap> aliases;

    static std::map<std::shared_ptr<const Symbol>, std::shared_ptr<Namespace> > namespaces;

    Namespace(std::shared_ptr<const Symbol> name) : AReference(name->meta()), name(name),
        mappings(NULL), aliases(NULL) /* TODO mappings.set(RT.DEFAULT_IMPORTS); aliases.set(RT.map()); */ {};
    
};

std::string Namespace::toString(void) const {
  return name->toString();
};

std::shared_ptr<const IMeta> Namespace::with_meta(std::shared_ptr<const IMap> meta) const {
  return std::dynamic_pointer_cast<const IMeta>(shared_from_this());
}

std::shared_ptr<Namespace> Namespace::findOrCreate(std::shared_ptr<const Symbol> name) {
  try {
    return namespaces.at(name);
  } catch(std::out_of_range) {
    std::shared_ptr<Namespace> newns = std::shared_ptr<Namespace>(new Namespace(name));
    namespaces[name] = newns;
    return newns;
  }
}

std::map<std::shared_ptr<const Symbol>, std::shared_ptr<Namespace> > Namespace::namespaces = {};   // TODO


// Var.cpp

class Var : public ARef /*, public IFn, public Settable*/ {
  public:
    virtual std::string toString(void) const;
    virtual std::shared_ptr<const IMeta> with_meta(std::shared_ptr<const IMap>) const;
    virtual std::shared_ptr<const lisp_object> deref();
};

std::string Var::toString(void) const {
  // TODO
}

std::shared_ptr<const IMeta> Var::with_meta(std::shared_ptr<const IMap>) const {
  // TODO
}

std::shared_ptr<const lisp_object> Var::deref() {
  // TODO
}


// Node.hpp
#include <functional>
#include <thread>
#include <vector>
class INode : public lisp_object {
  public:
    virtual std::string toString(void) const {return "";};
    std::shared_ptr<const INode> assoc(size_t shift, uint32_t hash, std::shared_ptr<const lisp_object> key, std::shared_ptr<const lisp_object> val, bool& addedLeaf) const {
      return std::shared_ptr<const INode>(assoc_impl(shift, hash, key, val, addedLeaf));
    };
	  std::shared_ptr<INode> assoc(size_t hashId, size_t shift, size_t hash, std::shared_ptr<const lisp_object> key, std::shared_ptr<const lisp_object> val, bool& addedLeaf) {
	    return std::shared_ptr<INode>(assoc_impl(hashId, shift, hash, key, val, addedLeaf));
	  };
	  virtual std::shared_ptr<INode> assoc(std::thread::id id, size_t shift, uint32_t hash, std::shared_ptr<const lisp_object> key, std::shared_ptr<const lisp_object> val, bool& addedLeaf) {return assoc(std::hash<std::thread::id>{}(id), shift, hash, key, val, addedLeaf);};
	  // virtual std::shared_ptr<INode> without(size_t shift, size_t hash, std::shared_ptr<const lisp_object> key) = 0;
	  // virtual std::shared_ptr<const IMapEntry> find(size_t shift, size_t hash, std::shared_ptr<const lisp_object> key) = 0;
	  // virtual std::shared_ptr<const lisp_object> find(size_t shift, size_t hash, std::shared_ptr<const lisp_object> key, std::shared_ptr<const lisp_object> notFound) = 0;

	// ISeq nodeSeq();
	// INode without(AtomicReference<Thread> edit, int shift, int hash, Object key, Box removedLeaf);
  // Object kvreduce(IFn f, Object init);
	// Object fold(IFn combinef, IFn reducef, IFn fjtask, IFn fjfork, IFn fjjoin);
	protected:
	  INode(size_t hashId, std::vector<std::shared_ptr<const lisp_object> > array) :
	    hashId(hashId), array(array) {};
	  std::vector<std::shared_ptr<const lisp_object> > array;
    size_t hashId;

	  virtual const INode *assoc_impl(size_t shift, uint32_t hash, std::shared_ptr<const lisp_object> key, std::shared_ptr<const lisp_object> val, bool& addedLeaf) const = 0;
	  virtual INode *assoc_impl(size_t hashId, size_t shift, size_t hash, std::shared_ptr<const lisp_object> key, std::shared_ptr<const lisp_object> val, bool& addedLeaf) = 0;

};

template<class Derived>
class INode_inherit : public INode {
  public:
    virtual ~INode_inherit() = default;
    std::shared_ptr<const INode> assoc(size_t shift, uint32_t hash, std::shared_ptr<const lisp_object> key, std::shared_ptr<const lisp_object> val, bool& addedLeaf) const {
      return std::shared_ptr<const INode>(assoc_impl(shift, hash, key, val, addedLeaf));
    };
    std::shared_ptr<INode> assoc(size_t hashId, size_t shift, size_t hash, std::shared_ptr<const lisp_object> key, std::shared_ptr<const lisp_object> val, bool& addedLeaf) {
	    return std::shared_ptr<INode>(assoc_impl(hashId, shift, hash, key, val, addedLeaf));
	  };
	protected:
	  virtual Derived *ensureEditable(size_t hashId) = 0;
	  Derived *editAndSet(size_t hashId, size_t i, std::shared_ptr<const lisp_object> a) {
	    Derived *editable = ensureEditable(hashId);
      editable->array[i] = a;
      return editable;
	  };
	  Derived *editAndSet(size_t hashId, size_t i, std::shared_ptr<const lisp_object> a, size_t j, std::shared_ptr<const lisp_object> b) {
	    Derived *editable = ensureEditable(hashId);
      editable->array[i] = a;
      editable->array[j] = b;
      return editable;
	  };
  private:
    INode_inherit() = default;
    INode_inherit(size_t hashId, std::vector<std::shared_ptr<const lisp_object> > array) :
      INode(hashId, array) {};
    friend Derived;
};


#define NODE_LOG_SIZE 5
#define NODE_SIZE (1 << NODE_LOG_SIZE)
#define NODE_BITMASK (NODE_SIZE - 1)

#include <limits.h>
#include <stdint.h>
#include <x86intrin.h>

#if INT32_MAX == INT_MAX
	#define popcount __builtin_popcount
#elif INT32_MAX == LONG_MAX
	#define popcount __builtin_popcountl
#elif INT32_MAX == LONG_LONG_MAX
	#define popcount __builtin_popcountll
#endif

static uint32_t mask(uint32_t hash, size_t shift) {
	return (hash >> shift) & NODE_BITMASK;
}

static uint32_t bitpos(uint32_t hash, size_t shift) {
	return 1 << mask(hash, shift);
}

class BitmapIndexedNode : public INode_inherit<BitmapIndexedNode> {
  public:
    BitmapIndexedNode(std::thread::id id, uint32_t bitmap, std::vector<std::shared_ptr<const lisp_object> > array) :
      INode_inherit<BitmapIndexedNode>(std::hash<std::thread::id>{}(id), array), bitmap(bitmap) {};
    BitmapIndexedNode(size_t hashId, uint32_t bitmap, std::vector<std::shared_ptr<const lisp_object> > array) :
      INode_inherit<BitmapIndexedNode>(hashId, array), bitmap(bitmap) {};

    size_t index(uint32_t bit) const;

    static const std::shared_ptr<const BitmapIndexedNode> Empty;
    static const std::thread::id NOEDIT;
    static const size_t HASH_NOEDIT;
  private:
    uint32_t bitmap;
    
    virtual const INode *assoc_impl(size_t shift, uint32_t hash, std::shared_ptr<const lisp_object> key, std::shared_ptr<const lisp_object> val, bool& addedLeaf) const;
	  virtual INode *assoc_impl(size_t hashId, size_t shift, size_t hash, std::shared_ptr<const lisp_object> key, std::shared_ptr<const lisp_object> val, bool& addedLeaf);
	  BitmapIndexedNode *ensureEditable(size_t hashId);
};

class HashCollisionNode : public INode_inherit<HashCollisionNode>, std::enable_shared_from_this<HashCollisionNode> {
  public:
    HashCollisionNode(size_t hashId, uint32_t hash, size_t count, std::vector<std::shared_ptr<const lisp_object> > array) :
      INode_inherit<HashCollisionNode>(hashId, array), hash(hash), count(count) {};
    
  private:
    const uint32_t hash;
    size_t count;

	  virtual HashCollisionNode *ensureEditable(size_t hashId);
    size_t findIndex(std::shared_ptr<const lisp_object>) const;
    virtual const INode *assoc_impl(size_t shift, uint32_t hash, std::shared_ptr<const lisp_object> key, std::shared_ptr<const lisp_object> val, bool& addedLeaf) const;
	  virtual INode *assoc_impl(size_t hashId, size_t shift, size_t hash, std::shared_ptr<const lisp_object> key, std::shared_ptr<const lisp_object> val, bool& addedLeaf);
};

class ArrayNode : public INode_inherit<ArrayNode> {
  public:
    ArrayNode(std::thread::id id, size_t count, std::vector<std::shared_ptr<INode> >array) :
      INode_inherit<ArrayNode>(std::hash<std::thread::id>{}(id), std::vector<std::shared_ptr<const lisp_object> >()),
      count(count), array(array) {};
    ArrayNode(size_t hashId, size_t count, std::vector<std::shared_ptr<INode> >array) :
      INode_inherit<ArrayNode>(hashId, std::vector<std::shared_ptr<const lisp_object> >()),
      count(count), array(array) {};

  private:
    size_t count;
    std::vector<std::shared_ptr<INode> >array;

    virtual const INode *assoc_impl(size_t shift, uint32_t hash, std::shared_ptr<const lisp_object> key, std::shared_ptr<const lisp_object> val, bool& addedLeaf) const;
    virtual INode *assoc_impl(size_t hashId, size_t shift, size_t hash, std::shared_ptr<const lisp_object> key, std::shared_ptr<const lisp_object> val, bool& addedLeaf);
	  virtual ArrayNode *ensureEditable(size_t hashId);

    ArrayNode *editAndSet(size_t hashId, size_t i, std::shared_ptr<INode> a);
};


static std::shared_ptr<const INode> createNode(size_t hashId, size_t shift, std::shared_ptr<const lisp_object> key1, std::shared_ptr<const lisp_object> val1, uint32_t key2hash, std::shared_ptr<const lisp_object> key2, std::shared_ptr<const lisp_object> val2) {
  size_t key1hash = hash(key1);
  if(key1hash == key2hash) {
      return std::make_shared<HashCollisionNode>(BitmapIndexedNode::HASH_NOEDIT, key1hash,
        2, std::vector<std::shared_ptr<const lisp_object> >{key1, val1, key2, val2});
    return NULL;
  }
  bool addedLeaf = false;
  return std::const_pointer_cast<BitmapIndexedNode>(BitmapIndexedNode::Empty)
    ->assoc(hashId, shift, key1hash, key1, val1, addedLeaf)
    ->assoc(hashId, shift, key2hash, key2, val2, addedLeaf);
}

static std::shared_ptr<const INode> createNode(size_t shift, std::shared_ptr<const lisp_object> key1, std::shared_ptr<const lisp_object> val1, uint32_t key2hash, std::shared_ptr<const lisp_object> key2, std::shared_ptr<const lisp_object> val2) {
  return createNode(BitmapIndexedNode::HASH_NOEDIT, shift, key1, val1, key2hash, key2, val2);
}

// BitmapIndexedNode
const std::thread::id BitmapIndexedNode::NOEDIT =  std::thread().get_id();
const std::shared_ptr<const BitmapIndexedNode> BitmapIndexedNode::Empty = std::make_shared<const BitmapIndexedNode>(NOEDIT, 0, std::vector<std::shared_ptr<const lisp_object> >());
const size_t BitmapIndexedNode::HASH_NOEDIT = std::hash<std::thread::id>{}(BitmapIndexedNode::NOEDIT);

const INode *BitmapIndexedNode::assoc_impl(size_t shift, uint32_t hash, std::shared_ptr<const lisp_object> key, std::shared_ptr<const lisp_object> val, bool& addedLeaf) const {
  uint32_t bit = bitpos(hash, shift);
	size_t idx = index(bit);
	if((bitmap & bit) != 0) {
	  std::shared_ptr<const lisp_object> keyOrNull = array[2*idx];
	  std::shared_ptr<const lisp_object> valOrNode = array[2*idx + 1];
	  if(keyOrNull == NULL) {
	    std::shared_ptr<const INode> n = std::dynamic_pointer_cast<const INode>(valOrNode);
	    n = n->assoc(shift + NODE_LOG_SIZE, hash, key, val, addedLeaf);
	    if(n == valOrNode)
	      return this;
	    std::vector<std::shared_ptr<const lisp_object> > newArray(array);
	    newArray[2*idx+1] = n;
	    return new BitmapIndexedNode(NOEDIT, bitmap, newArray);
	  }
	  if(equiv(key, keyOrNull)) {
	    if(val == valOrNode)
	      return this;
	    std::vector<std::shared_ptr<const lisp_object> > newArray(array);
	    newArray[2*idx+1] = val;
	    return new BitmapIndexedNode(NOEDIT, bitmap, newArray);
	  }
	  addedLeaf = true;
    std::vector<std::shared_ptr<const lisp_object> > newArray(array);
	  newArray[2*idx    ] = NULL;
	  newArray[2*idx + 1] = createNode(shift + NODE_LOG_SIZE, keyOrNull, valOrNode, hash, key, val);
	  return new BitmapIndexedNode(NOEDIT, bitmap, newArray);
	} else {
	  size_t n = popcount(bitmap);
	  if(n >= (NODE_SIZE >> 1)) {
	    std::vector<std::shared_ptr<INode> > nodes(NODE_SIZE);
	    size_t jdx = mask(hash, shift);
	    nodes[jdx] = std::const_pointer_cast<INode>(Empty->assoc(shift + NODE_LOG_SIZE, hash, key, val, addedLeaf));
	    size_t j = 0;
	    for(size_t i=0; i<NODE_SIZE; i++) {
	      if((bitmap >> i) & 1) {
					if(array[j])
						nodes[i] = std::const_pointer_cast<INode>(Empty->assoc(shift + NODE_LOG_SIZE, hashEq(array[j]), array[j], array[j+1], addedLeaf));
					else
					  nodes[i] = std::const_pointer_cast<INode>(std::dynamic_pointer_cast<const INode>(array[j+1]));
					j += 2;
				}
	    }
	    return new ArrayNode(NOEDIT, n+1, nodes);
	  } else {
	    std::vector<std::shared_ptr<const lisp_object> > newArray(2*(n+1));
	    for(size_t i=0; i<2*idx; i++)
	      newArray[i] = array[i];
	    newArray[2*idx  ] = key;
	    addedLeaf = true;
	    newArray[2*idx+1] = val;
	    for(size_t i=2*idx; i<2*(n+1); i++)
	      newArray[i+2] = array[i];
	    return new BitmapIndexedNode(NOEDIT, bitmap | bit, newArray);
	  }
	}
}

INode *BitmapIndexedNode::assoc_impl(size_t hashId, size_t shift, size_t hash, std::shared_ptr<const lisp_object> key, std::shared_ptr<const lisp_object> val, bool& addedLeaf) {
  uint32_t bit = bitpos(hash, shift);
	size_t idx = index(bit);
	if(bitmap & bit) {
	  std::shared_ptr<const lisp_object> keyOrNull = array[2*idx];
	  std::shared_ptr<const lisp_object> valOrNode = array[2*idx+1];
	  if(keyOrNull == NULL) {
	    std::shared_ptr<INode> n = std::const_pointer_cast<INode>(std::dynamic_pointer_cast<const INode>(valOrNode));
	    n = n->assoc(hashId, shift + NODE_LOG_SIZE, hash, key, val, addedLeaf);
	    if(n == valOrNode)
	      return this;
	    return editAndSet(hashId, 2*idx+1, val);
	  }
	  if(equiv(key, keyOrNull)) {
	    if(val == valOrNode)
	      return this;
	    return editAndSet(hashId, 2*idx+1, val);
	  }
	  addedLeaf = true;
	  return editAndSet(hashId, 2*idx, NULL, 2*idx+1, 
	                    createNode(hashId, shift + NODE_LOG_SIZE, keyOrNull, valOrNode, hash, key, val));
	} else {
	  size_t n = popcount(bitmap);
	  if(2*n < array.size()) {
	    addedLeaf = true;
	    BitmapIndexedNode *editable = ensureEditable(hashId);
	    for(size_t i=2*n-1; i>=2*idx; i--)
	      editable->array[i+2] = editable->array[i];
	    editable->array[2*idx  ] = key;
			editable->array[2*idx+1] = val;
			editable->bitmap |= bit;
			return editable;
	  }
	  if(n >= (NODE_SIZE >> 1)) {
	    std::vector<std::shared_ptr<INode> > nodes(NODE_SIZE);
	    size_t jdx = mask(hash, shift);
	    nodes[jdx] = std::const_pointer_cast<BitmapIndexedNode>(Empty)->assoc(hashId, shift + NODE_LOG_SIZE, hash, key, val, addedLeaf);
	    size_t j = 0;
	    for(size_t i=0; i<NODE_SIZE; i++) {
				if((bitmap >> i) & 1) {
					if (array[j] == NULL)
						nodes[i] = std::const_pointer_cast<INode>(std::dynamic_pointer_cast<const INode>(array[j+1]));
					else
						nodes[i] = std::const_pointer_cast<BitmapIndexedNode>(Empty)->assoc(hashId, shift + NODE_LOG_SIZE, hashEq(array[j]), array[j], array[j+1], addedLeaf);
					j += 2;
				}
	    }
	    return new ArrayNode(hashId, n+1, nodes);
	  } else {
	    std::vector<std::shared_ptr<const lisp_object> > newArray(2*(n+4));
	    for(size_t i=0; i<2*idx; i++)
	      newArray[i] = array[i];
	    newArray[2*idx] = key;
			addedLeaf = true;
			newArray[2*idx+1] = val;
			for(size_t i=2*idx; i<2*n; i++)
			  newArray[i+2] = array[i];
			 BitmapIndexedNode *editable = ensureEditable(hashId);
			 editable->array = newArray;
			 editable->bitmap |= bit;
			 return editable;
	  }
	}
}

size_t BitmapIndexedNode::index(uint32_t bit) const {
	return popcount(bitmap & (bit - 1));
}

BitmapIndexedNode *BitmapIndexedNode::ensureEditable(size_t hashId) {
  if(this->hashId == hashId)
    return this;
  size_t n = popcount(bitmap);
  std::vector<std::shared_ptr<const lisp_object> > newArray(2*(n+1));
  for(size_t i=0; i<2*n; i++)
    newArray[i] = array[i];
  return new BitmapIndexedNode(hashId, bitmap, newArray);
}

// HashCollisionNode
const INode *HashCollisionNode::assoc_impl(size_t shift, uint32_t hash, std::shared_ptr<const lisp_object> key, std::shared_ptr<const lisp_object> val, bool& addedLeaf) const {
  if(hash == this->hash) {
    size_t idx = findIndex(key);
    if(idx != (size_t)-1) {
      if(array[idx+1]->equals(val))
        return this;
    }
    std::vector<std::shared_ptr<const lisp_object> > newArray(2*count + 2);
    for(size_t i=0; i<2*count+2; i++)
      newArray[i] = array[i];
    newArray[2*count    ] = key;
    newArray[2*count + 1] = val;
    addedLeaf = true;
    return new HashCollisionNode(hashId, hash, count+1, newArray);
  }
  return (new BitmapIndexedNode(BitmapIndexedNode::NOEDIT, bitpos(this->hash, shift), std::vector<std::shared_ptr<const lisp_object> >{NULL, std::dynamic_pointer_cast<const INode>(shared_from_this())}))
    ->assoc(shift, hash, key, val, addedLeaf).get();
}

INode *HashCollisionNode::assoc_impl(size_t hashId, size_t shift, size_t hash, std::shared_ptr<const lisp_object>   key, std::shared_ptr<const lisp_object> val, bool& addedLeaf) {
  if(hash == this->hash) {
    // TODO
  }
  // TODO
  return NULL;
}

size_t HashCollisionNode::findIndex(std::shared_ptr<const lisp_object> key) const {
  for(size_t i=0; i<array.size(); i+=2)
    if(equiv(key, array[i]))
      return i;
  return (size_t) -1;
}

HashCollisionNode *HashCollisionNode::ensureEditable(size_t hashId) {
  if(this->hashId == hashId)
    return this;
  std::vector<std::shared_ptr<const lisp_object> > newArray(2*(count+1));
  for(size_t i=0; i<2*count; i++)
    newArray[i] = array[i];
  return new HashCollisionNode(hashId, hash, count, newArray);
}

// ArrayNode
const INode *ArrayNode::assoc_impl(size_t shift, uint32_t hash, std::shared_ptr<const lisp_object> key, std::shared_ptr<const lisp_object> val, bool& addedLeaf) const {
  size_t idx = mask(hash, shift);
	std::shared_ptr<INode> node = array[idx];
	if(node == NULL) {
	  std::vector<std::shared_ptr<INode> >newArray(array);
	  newArray[idx] = std::const_pointer_cast<INode>(BitmapIndexedNode::Empty->assoc(shift + NODE_LOG_SIZE, hash, key, val, addedLeaf));
		return new ArrayNode(BitmapIndexedNode::NOEDIT, count + 1, newArray);
	}
	std::shared_ptr<INode> n = std::const_pointer_cast<INode>(node->assoc(shift + NODE_LOG_SIZE, hash, key, val, addedLeaf));
  if(n == node)
    return this;
  std::vector<std::shared_ptr<INode> >newArray(array);
	newArray[idx] = n;
	return new ArrayNode(BitmapIndexedNode::NOEDIT, count, newArray);
}

INode *ArrayNode::assoc_impl(size_t hashId, size_t shift, size_t hash, std::shared_ptr<const lisp_object> key, std::shared_ptr<const lisp_object> val, bool& addedLeaf) {
  size_t idx = mask(hash, shift);
  std::shared_ptr<INode> node = array[idx];
  if(node == NULL) {
    ArrayNode *editable = editAndSet(hashId, idx, std::const_pointer_cast<BitmapIndexedNode>(BitmapIndexedNode::Empty)
        ->assoc(hashId, shift+NODE_LOG_SIZE, hash, key, val, addedLeaf));
      editable->count++;
      return editable;
  }
  std::shared_ptr<INode> n = node->assoc(hashId, shift+NODE_LOG_SIZE, hash, key, val, addedLeaf);
  if(n == node)
    return this;
  return editAndSet(hashId, idx, n);
}

ArrayNode *ArrayNode::ensureEditable(size_t hashId) {
  if(this->hashId == hashId)
    return this;
  return new ArrayNode(hashId, count, array);
}

ArrayNode *ArrayNode::editAndSet(size_t hashId, size_t i, std::shared_ptr<INode> a) {
  ArrayNode *editable = ensureEditable(hashId);
  editable->array[i] = a;
  return editable;
};


// Map.cpp

size_t LMap::count() const {
  return _count;
}

std::shared_ptr<const ISeq> LMap::seq() const {
  // TODO
}

std::shared_ptr<const ICollection> LMap::empty() const {
  return std::dynamic_pointer_cast<const ICollection>(EMPTY->with_meta(meta()));
}

const LMap *LMap::assoc_impl(std::shared_ptr<const lisp_object> key, std::shared_ptr<const lisp_object> val) const {
  if(key == NULL) {
    if(hasNull && (val == nullValue))
      return this;
    return new LMap(meta(), _count + (hasNull ? 0 : 1), root, true, val);
  }
  bool addedLeaf = false;
  std::shared_ptr<const INode> newroot = (root == NULL ? BitmapIndexedNode::Empty : root)->assoc(0, hashEq(key), key, val, addedLeaf);
  if(newroot == root)
    return this;
  return new LMap(meta(), addedLeaf ? _count + 1 : _count, newroot, hasNull, nullValue);
}

//    LMap(std::shared_ptr<const IMap> meta, size_t count, std::shared_ptr<const INode> root, bool hasNull, std::shared_ptr<const lisp_object> nullValue) : IMeta(meta), _count(count), root(root), hasNull(hasNull), nullValue(nullValue) {};

std::shared_ptr<const IMap> LMap::without(std::shared_ptr<const lisp_object>) const {
  // TODO
}

std::shared_ptr<const IMeta> LMap::with_meta(std::shared_ptr<const IMap> meta) const {
  return std::shared_ptr<const LMap>(new LMap(meta, _count, root, hasNull, nullValue));
}

std::shared_ptr<ITransientCollection> LMap::asTransient() const {
  return std::make_shared<TransientMap>(std::hash<std::thread::id>{}(std::this_thread::get_id()), _count, std::const_pointer_cast<INode>(root), hasNull, nullValue);
}

std::shared_ptr<const LMap> LMap::EMPTY = std::shared_ptr<LMap>(new LMap(0, NULL, false, NULL));

std::string TransientMap::toString() const {
  // TODO
}

size_t TransientMap::count() const {
  ensureEditable();
  // TODO
}

std::shared_ptr<ITransientCollection> TransientMap::conj(std::shared_ptr<const lisp_object>) {
  ensureEditable();
  // TODO
}

std::shared_ptr<const ICollection> TransientMap::persistent() {
  ensureEditable();
  hashId = BitmapIndexedNode::HASH_NOEDIT;
  return std::shared_ptr<const LMap>(new LMap(_count, root, hasNull, nullValue));
  // TODO
}

std::shared_ptr<ITransientMap> TransientMap::without(std::shared_ptr<const lisp_object> key) {
  ensureEditable();
  // TODO
}

TransientMap *TransientMap::assoc_impl(std::shared_ptr<const lisp_object> key, std::shared_ptr<const lisp_object> val) {
  ensureEditable();
  if (key == NULL) {
    if (nullValue != val)
			nullValue = val;
		if (!hasNull) {
			_count++;
			hasNull = true;
		}
		return this;
  }
	bool leafFlag = false;
	std::shared_ptr<INode> n = (root == NULL ? std::const_pointer_cast<BitmapIndexedNode>(BitmapIndexedNode::Empty) : root)
		->assoc(hashId, 0, hashEq(key), key, val, leafFlag);
	if (n != root)
		root = n; 
	if(leafFlag) _count++;
	return this;
}

void TransientMap::ensureEditable() const {
  if(hashId == BitmapIndexedNode::HASH_NOEDIT)
    throw std::runtime_error("Transient used after persistent! call");
}


int main() {
  std::cout << "Hello World!\n";
}
















