#include <iostream>

#include <cassert>
#include <map>
#include <memory>
#include <string>

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
    // virtual bool equiv(std::shared_ptr<const lisp_object>)const = 0;
};
class Associative2 : virtual public lisp_object {
  public:
    // virtual bool containsKey(std::shared_ptr<const lisp_object>) const = 0;
    // virtual std::shared_ptr<const lisp_object> entryAt(std::shared_ptr<const lisp_object>) const = 0;

};
class Associative : virtual public ICollection, virtual public ILookup, virtual public Associative2 {
  public:
    virtual std::shared_ptr<const Associative> assoc(std::shared_ptr<const lisp_object>,
                                                     std::shared_ptr<const lisp_object>) const = 0;
};

class IMap : public virtual Associative {
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


// Util.cpp

std::shared_ptr<const lisp_object> ret1(std::shared_ptr<const lisp_object> ret, std::shared_ptr<const lisp_object>) {
  return ret;
}

std::shared_ptr<const ISeq> ret1(std::shared_ptr<const ISeq> ret, std::shared_ptr<const lisp_object>) {
  return ret;
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


// Map.cpp
class LMap {
  public:
    static std::shared_ptr<const IMap> EMPTY;
};
std::shared_ptr<const IMap> LMap::EMPTY = NULL;

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

int main() {
  Var v;
  std::cout << "Hello World!\n";
}
















