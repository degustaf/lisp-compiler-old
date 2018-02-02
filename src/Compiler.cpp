#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <thread>
#include <typeinfo>
#include <vector>

class LineNumberIStream : public std::istream {
  public:
    LineNumberIStream(std::istream &is) : is(is), line(0), column(0), atLineStart(false), prevAtLineStart(false) {};
    LineNumberIStream(const char *filename) : is_buf(std::unique_ptr<std::istream>(new std::ifstream(filename))),
      is(*is_buf), line(1), column(1), atLineStart(true), prevAtLineStart(false) {};
    LineNumberIStream(std::string &buffer) : is_buf(std::unique_ptr<std::istream>(new std::istringstream(buffer))),
      is(*is_buf), line(0), column(0), atLineStart(true), prevAtLineStart(false) {};
    size_t getLineNumber(void) const;
    size_t getColumnNumber(void) const;
    int get(void);
    void unget(int);
  private:
	  std::istream &is;
    std::unique_ptr<std::istream> is_buf;
    size_t line;
  	size_t column;
  	bool atLineStart;
  	bool prevAtLineStart;
};

class IMap;
class ISeq;

class lisp_object : public std::enable_shared_from_this<lisp_object> {
  public:
    virtual std::string toString(void) {return ((lisp_object * const) this)->toString();};
    virtual std::string toString(void) const = 0;
    virtual std::shared_ptr<const lisp_object> with_meta(std::shared_ptr<const IMap>) const {
      return shared_from_this(); // TODO
    };
    std::shared_ptr<const IMap> meta(void) const {return NULL;};
    virtual bool equals(const lisp_object &o) const {return &o == this;};
    bool operator==(const lisp_object &o) const {return equals(o);};
    virtual operator bool() const {return true;};
  protected:
    lisp_object(void) {};
    lisp_object(std::shared_ptr<const IMap>) {};
    std::shared_ptr<const IMap> _meta;
};
std::string toString(std::shared_ptr<const lisp_object>) {return "";};
class IType : public lisp_object {
};
class IPending {
    // bool isRealized();
};
class IHashEq{
  // int hasheq();
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
class IReference : public lisp_object {
  public:
    // virtual std::shared_ptr<const IMap> alterMeta(IFn alter, ISeq args) = 0;
    virtual std::shared_ptr<const IMap> resetMeta(std::shared_ptr<const IMap> m) = 0;
  protected:
    IReference() {};
    IReference(std::shared_ptr<const IMap> meta) : lisp_object(meta) {};
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
    virtual std::shared_ptr<const lisp_object> valAt(std::shared_ptr<const lisp_object> key) const = 0;
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
    virtual std::shared_ptr<const ICollection>cons(std::shared_ptr<const lisp_object>)const = 0;
    virtual std::shared_ptr<const ICollection>empty()const = 0;
    // virtual bool equiv(std::shared_ptr<const lisp_object>)const = 0;
};
class ISet : public ICollection {
  public:
    virtual std::shared_ptr<const ISet> disjoin(std::shared_ptr<const lisp_object> key) const = 0;
	  virtual bool contains(std::shared_ptr<const lisp_object> key) const = 0;
	  virtual std::shared_ptr<const lisp_object> get(std::shared_ptr<const lisp_object> key) const = 0;
};
class Associative2 : virtual public lisp_object {
  public:
    virtual bool containsKey(std::shared_ptr<const lisp_object>) const = 0;
    // virtual std::shared_ptr<const lisp_object> entryAt(std::shared_ptr<const lisp_object>) const = 0;

};
class Associative : virtual public ICollection, virtual public ILookup, virtual public Associative2 {
  public:
    virtual std::shared_ptr<const Associative> assoc(std::shared_ptr<const lisp_object>,
                                                     std::shared_ptr<const lisp_object>) const = 0;
};
class Indexed : virtual public Counted {
  public:
    // virtual std::shared_ptr<const lisp_object>nth(size_t i)const = 0;
    // virtual std::shared_ptr<const lisp_object>nth(size_t i, std::shared_ptr<const lisp_object> NotFound)const = 0;
};
class IStack : virtual public ICollection {
  public:
    // virtual std::shared_ptr<const lisp_object>peek()const = 0;
    // virtual std::shared_ptr<const IStack>pop()const = 0;
};
class Reversible : virtual public lisp_object {
  public:
    // virtual std::shared_ptr<const lisp_object> rseq(void) const = 0;
};
class IMap : public virtual Associative {
  public:
    // virtual std::shared_ptr<const IMap> assocEx(std::shared_ptr<const lisp_object>,
    //                                                  std::shared_ptr<const lisp_object>) const = 0;
    virtual std::shared_ptr<const IMap> without(std::shared_ptr<const lisp_object>) const = 0;
};
class IVector : virtual public Associative, virtual public IStack, virtual public Reversible, virtual public Indexed {
  public:
    virtual size_t length (void) const = 0;
    // virtual std::shared_ptr<const IVector>assocN(size_t n, std::shared_ptr<const lisp_object> val) const = 0;
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

class Boolean : public lisp_object {
  public:
    Boolean(bool tf) : tf(tf) {};
    virtual std::string toString(void) const {
      if(tf)
        return "true";
      return "false";
    };
    virtual operator bool() const {return tf;};
  private:
    bool tf;
};
const std::shared_ptr<const Boolean> T = std::make_shared<const Boolean>(true);
const std::shared_ptr<const Boolean> F = std::make_shared<const Boolean>(false);

class lisp_String : public lisp_object {
  public:
    lisp_String(std::string s) : s(s) {};
    virtual std::string toString(void) const {return s;};
  private:
    std::string s;
};

// Numbers.cpp
class Number : public lisp_object {
  public:
    // virtual operator char() const = 0;
    // virtual operator int() const = 0;
    // virtual operator short() const = 0;
    virtual operator long() const = 0;
    // virtual operator float() const = 0;
    virtual operator double() const = 0;
};

class Integer : public Number {
  public:
    Integer(long val) : val(val) {};

    virtual operator long() const {return val;};
    virtual operator double() const {return (double) val;};
    virtual std::string toString(void) const {
      return std::to_string(val);
    }
  private:
    const long val;
};

// List.cpp
class List : virtual public ISeq {
  public:
    List(std::shared_ptr<const lisp_object> first) : _first(first), _rest(NULL), _count(1) {};
    List(std::shared_ptr<const IMap> meta, std::shared_ptr<const lisp_object> first, std::shared_ptr<const ISeq> rest, size_t count) : _first(first), _rest(rest), _count(count) {};
    List(std::vector<std::shared_ptr<const lisp_object> >& entries) : _first(entries[0]), _rest(createRest(entries)), _count(entries.size()) {};
    virtual std::string toString(void) const {return "";};
    virtual std::shared_ptr<const ISeq>next(void) const {return NULL;};
    virtual std::shared_ptr<const lisp_object>first(void) const {return NULL;};
    virtual std::shared_ptr<const ICollection>cons(std::shared_ptr<const lisp_object>)const {
      return std::dynamic_pointer_cast<const ISeq>(shared_from_this());  // TODO
    };
    virtual size_t count(void) const {};  // TODO
    virtual std::shared_ptr<const ISeq> seq() const {}; // TODO
    virtual std::shared_ptr<const ICollection>empty() const {return EMPTY;};
    virtual std::shared_ptr<const ISeq> more() const {};  // TODO

    static const std::shared_ptr<const ISeq> EMPTY;
  private:
    static std::shared_ptr<const ISeq>createRest(std::vector<std::shared_ptr<const lisp_object> >& entries);
    std::shared_ptr<const lisp_object> _first;
    std::shared_ptr<const ISeq> _rest;
    const size_t _count;
    
};
const std::shared_ptr<const ISeq> List::EMPTY = std::make_shared<const List>(std::shared_ptr<const lisp_object>(NULL));

// Vector.cpp
class AVector : virtual public IVector /* virtual public AFn */ {
  public:
    virtual std::string toString(void) const {
      return ::toString(shared_from_this());
    }
    virtual size_t length (void) const {return count();};
};

#define LOG_NODE_SIZE 5
#define NODE_SIZE (2 << LOG_NODE_SIZE)

class Node : virtual public lisp_object {
  public:
    Node(std::thread::id id) : id(std::hash<std::thread::id>{}(id)), array(std::vector<std::shared_ptr<const lisp_object> >(NODE_SIZE)) {};
    Node(std::thread::id id, std::vector<std::shared_ptr<const lisp_object> > array) : id(std::hash<std::thread::id>{}(id)), array(array) {};
    Node(size_t id) : id(id), array(std::vector<std::shared_ptr<const lisp_object> >(NODE_SIZE)) {};
    Node(size_t id, std::vector<std::shared_ptr<const lisp_object> > array) : id(id), array(array) {};
    virtual std::string toString(void) const {return "";};
    
    // const std::shared_ptr<std::thread::id> id;
    const size_t id;
    // std::mutex id_mutex;
    const std::vector<std::shared_ptr<const lisp_object> > array;

    static const std::thread::id NOEDIT;
    static const std::shared_ptr<const Node> EmptyNode;
};
const std::thread::id Node::NOEDIT = std::thread().get_id();
const std::shared_ptr<const Node> Node::EmptyNode(new Node(NOEDIT));

class LVector : virtual public AVector {
  public:
    // static std::shared_ptr<const LVector> create(std::vector<std::shared_ptr<const lisp_object> >);
    virtual std::shared_ptr<const ICollection>empty(void) const {}; // TODO
    virtual size_t count(void) const {return cnt;};
    virtual std::shared_ptr<const lisp_object> valAt(std::shared_ptr<const lisp_object> key) const {};  // TODO
    virtual bool containsKey(std::shared_ptr<const lisp_object>) const {};   // TODO
    virtual std::shared_ptr<const ISeq> seq() const {};   // TODO
    virtual std::shared_ptr<const ICollection>cons(std::shared_ptr<const lisp_object>)const {};   // TODO
    virtual std::shared_ptr<const Associative> assoc(std::shared_ptr<const lisp_object>,
                                                     std::shared_ptr<const lisp_object>) const {};    // TODO

    static const std::shared_ptr<const LVector> EMPTY;
  private:
    const size_t cnt;
    const size_t shift;
    const std::shared_ptr<const Node> root;
    const std::vector<std::shared_ptr<const lisp_object> > tail;
    
    LVector(size_t cnt, size_t shift, std::shared_ptr<const Node> root, 
      const std::vector<std::shared_ptr<const lisp_object> > tail) : 
      cnt(cnt), shift(shift), root(root), tail(tail) {};
    LVector(std::shared_ptr<const IMap>meta, size_t cnt, size_t shift, std::shared_ptr<const Node> root, 
      const std::vector<std::shared_ptr<const lisp_object> > tail) : 
      lisp_object(meta), cnt(cnt), shift(shift), root(root), tail(tail) {};
};
const std::shared_ptr<const LVector> LVector::EMPTY(new LVector(0, LOG_NODE_SIZE, Node::EmptyNode,
                                                    std::vector<std::shared_ptr<const lisp_object> >()));


// Map.cpp
class LMap {
  public:
    static std::shared_ptr<const IMap> EMPTY;
};
std::shared_ptr<const IMap> LMap::EMPTY = NULL;

// Set.cpp
class LSet : public ISet {
  public:
    static std::shared_ptr<const LSet> EMPTY;
  private:
};
std::shared_ptr<const LSet> LSet::EMPTY = NULL;

// AReference.hpp
class AReference : public IReference {
  public:
    virtual std::shared_ptr<const IMap> resetMeta(std::shared_ptr<const IMap> m) {lisp_object::_meta = m; return m;};
  protected:
    AReference() {};
    AReference(std::shared_ptr<const IMap> meta) : IReference(meta) {};
};

// ARef.cpp
class ARef : public AReference, public IRef {
  public:
    ARef() : validator(NULL), watches(LMap::EMPTY) {};
    ARef(std::shared_ptr<const IMap> meta) : AReference(meta), validator(NULL), watches(LMap::EMPTY) {};

    virtual void setValidator(std::shared_ptr<const IFn>) {};   // TODO
    virtual std::shared_ptr<const IFn> getValidator() {};   // TODO
    virtual std::shared_ptr<const IMap> getWatches() {};   // TODO
    virtual std::shared_ptr<IRef> addWatch(std::shared_ptr<const lisp_object>, std::shared_ptr<const IFn>) {};   // TODO
    virtual std::shared_ptr<IRef> removeWatch(std::shared_ptr<const lisp_object>) {};   // TODO
    void notifyWatches(std::shared_ptr<const lisp_object> oldval, std::shared_ptr<const lisp_object> newval) {};   // TODO
protected:
    std::shared_ptr<const IFn> validator;
  private:
    std::shared_ptr<const IMap> watches;
    
    void validate(std::shared_ptr<const IFn>, std::shared_ptr<const lisp_object>);
};

// Var.cpp
class Var : public ARef /*, public IFn, public Settable*/ {
  public:
    virtual std::string toString(void) const {};   // TODO
    virtual std::shared_ptr<const lisp_object> deref() {};   // TODO
    std::shared_ptr<Var> setDynamic() {
      return std::dynamic_pointer_cast<Var>(shared_from_this());
    };
    bool isBound() const {};    // TODO
    virtual std::shared_ptr<const lisp_object> get() {};    // TODO

    static std::shared_ptr<Var> create(std::shared_ptr<const lisp_object>){
      return std::make_shared<Var>(); // TODO
    };
    static void pushThreadBindings(std::shared_ptr<const Associative> bindings) {};  // TODO
    static void popThreadBindings() {};   // TODO
};

// Symbol.cpp
class Symbol : /* public AFn, */ public lisp_object, public Named, public Comparable {
  public:
    virtual std::string toString(void) const {};  // TODO
    virtual std::string getName() const {return name;};
    virtual std::string getNamespace() const {return ns;};
    virtual bool equals(std::shared_ptr<const lisp_object>) const {};  // TODO
    virtual int compare(std::shared_ptr<const lisp_object>) const {};  // TODO
    bool operator< (std::shared_ptr<const lisp_object> o) {return Comparable::operator<(o);};

    static std::shared_ptr<Symbol> intern(std::string nsname) {
      size_t i = nsname.find('/');
      if(i == std::string::npos || nsname == "/")
        return std::shared_ptr<Symbol>(new Symbol("", nsname));
      return std::shared_ptr<Symbol>(new Symbol(nsname.substr(0,i), nsname.substr(i+1)));
    };
    static std::shared_ptr<Symbol> intern(std::string ns, std::string name) {
      return std::shared_ptr<Symbol>(new Symbol(ns, name));
    };
  private:
    const std::string ns;
    const std::string name;
    
    Symbol(std::string ns, std::string name) : ns(ns), name(name) {};
    Symbol(std::shared_ptr<const IMap> meta, std::string ns, std::string name) : lisp_object(meta), ns(ns), name(name) {};
};

// Keyword.cpp
class Keyword : public Named, public Comparable, public lisp_object /* IFn, IHashEq */ {
  public:
    virtual std::string toString(void) const {};  // TODO
    virtual std::string getName() const {};  // TODO
    virtual std::string getNamespace() const {};  // TODO
    virtual int compare(std::shared_ptr<const lisp_object> o) const {};  // TODO

    static std::shared_ptr<Keyword> intern(std::shared_ptr<const Symbol> sym){
      std::shared_ptr<Keyword> k = std::shared_ptr<Keyword>(new Keyword(sym));
      return k;
    };
    static std::shared_ptr<Keyword> intern(std::string nsname) {
      return intern(Symbol::intern(nsname));
    };
    static std::shared_ptr<Keyword> intern(std::string ns, std::string name) {
       return intern(Symbol::intern(ns, name));
    };
    // static std::shared_ptr<Keyword> find(std::shared_ptr<const Symbol> sym);
    // static std::shared_ptr<Keyword> find(std::string nsname);
    // static std::shared_ptr<Keyword> find(std::string ns, std::string name);

    const std::shared_ptr<const Symbol> sym;
  private:
    // static std::map<std::shared_ptr<const Symbol>, std::weak_ptr<Keyword> > table;
    Keyword(std::shared_ptr<const Symbol> sym) : sym(sym) {};
};

// Util.cpp
bool isPrimitive(const std::type_info *t) {
  if(t == NULL)
    return false;
	return (*t == typeid(Integer)) || /*(*t == typeid(Float)) ||*/ (*t == typeid(Boolean));
}

// RT.cpp
class RT {
  public:
    static const std::shared_ptr<const Keyword> LINE_KEY;
    static const std::shared_ptr<const Keyword> COLUMN_KEY;
    static const std::shared_ptr<const Keyword> TAG_KEY;
    
    static std::shared_ptr<const lisp_object> first(std::shared_ptr<const lisp_object> o) {
      return o;   // TODO
    };
    static std::shared_ptr<const lisp_object> list() {return NULL;};
    static std::shared_ptr<const lisp_object> list(std::shared_ptr<const lisp_object> o1) {
      return std::make_shared<List>(o1);
    };
    static std::shared_ptr<const lisp_object> list(std::shared_ptr<const lisp_object> o1,
                                                   std::shared_ptr<const lisp_object> o2) {
      return std::make_shared<List>(o2)->cons(o1);
    };
    static std::shared_ptr<const lisp_object> list(std::shared_ptr<const lisp_object> o1,
                                                   std::shared_ptr<const lisp_object> o2,
                                                   std::shared_ptr<const lisp_object> o3) {
      return std::make_shared<List>(o3)->cons(o2)->cons(o1);
    };
    static int nextID(){return 0;}; // TODO
    static std::shared_ptr<const ISeq> seq(std::shared_ptr<const lisp_object>) {return List::EMPTY;};  // TODO
    static std::shared_ptr<const ICollection> cons(std::shared_ptr<const lisp_object> x,
                                                   std::shared_ptr<const lisp_object> coll) {};  // TODO
    static std::shared_ptr<const lisp_object> get(std::shared_ptr<const lisp_object> coll,
                                                  std::shared_ptr<const lisp_object> key);
    static std::shared_ptr<const Associative> assoc(std::shared_ptr<const lisp_object> coll,
                                                    std::shared_ptr<const lisp_object> key,
                                                    std::shared_ptr<const lisp_object> val) {};   // TODO
};

const std::shared_ptr<const Keyword> RT::LINE_KEY = Keyword::intern("", "line");
const std::shared_ptr<const Keyword> RT::COLUMN_KEY = Keyword::intern("", "column");
const std::shared_ptr<const Keyword> RT::TAG_KEY = Keyword::intern("", "tag");

std::shared_ptr<const lisp_object> RT::get(std::shared_ptr<const lisp_object> coll,
                                           std::shared_ptr<const lisp_object> key) {
  if(coll == NULL)
    return NULL;
  std::shared_ptr<const ILookup> il = std::dynamic_pointer_cast<const ILookup>(coll);
  if(il)
    return il->valAt(key);
  std::shared_ptr<const ISet> is = std::dynamic_pointer_cast<const ISet>(coll);
  if(is)
    return is->get(key);
  // if(key instanceof Number && (coll instanceof String || coll.getClass().isArray()))   // TODO
	// if(coll instanceof ITransientSet)    // TODO
	return NULL;
}

// LazySeq.cpp
class LazySeq : public ISeq, public IPending, public IHashEq {
  public:
    LazySeq(std::shared_ptr<const IFn> fn) : fn(fn) {};
    virtual std::shared_ptr<const lisp_object> with_meta(std::shared_ptr<const IMap>) const;
    virtual std::shared_ptr<const ISeq> seq() const;
    virtual std::string toString(void) const;
    virtual size_t count(void) const;
    virtual std::shared_ptr<const ICollection> empty() const;
    virtual std::shared_ptr<const lisp_object> first() const;
    virtual std::shared_ptr<const ISeq> next() const;
    virtual std::shared_ptr<const ISeq> more() const;
    virtual std::shared_ptr<const ICollection>cons(std::shared_ptr<const lisp_object>)const;
    
    std::shared_ptr<const lisp_object> sval();
  private:
    std::shared_ptr<const IFn> fn;
    std::shared_ptr<const lisp_object> sv;
    std::shared_ptr<const ISeq> s;
    
    LazySeq(std::shared_ptr<const IMap> m, std::shared_ptr<const ISeq> s) : lisp_object(m), s(s) {};
};

std::shared_ptr<const lisp_object> LazySeq::with_meta(std::shared_ptr<const IMap> m) const {
	return std::shared_ptr<LazySeq>(new LazySeq(m, seq()));
}

std::shared_ptr<const ISeq> LazySeq::seq() const {
  std::shared_ptr<LazySeq> ls = std::make_shared<LazySeq>(*this);
  if(ls->sv) {
    std::shared_ptr<const lisp_object> o;
    std::shared_ptr<LazySeq> ls2;
    do {
      o = ls->sv ? ls->sv : ls->s;
      ls2 = std::const_pointer_cast<LazySeq>(std::dynamic_pointer_cast<const LazySeq>(o));
    } while(ls2);
    ls->s = RT::seq(o);
  }
  return ls->s;
}

std::string LazySeq::toString(void) const {
  return ::toString(shared_from_this());
}

size_t LazySeq::count(void) const {
  size_t c = 0;
	for(std::shared_ptr<const ISeq> s = seq(); s != NULL; s = s->next())
		c++;                                                                                
	return c;
}

std::shared_ptr<const ICollection> LazySeq::empty() const {
  return List::EMPTY;
}

std::shared_ptr<const lisp_object> LazySeq::first() const {
  std::shared_ptr<const ISeq> s = seq();
	if(s == NULL)
		return NULL;
	return s->first();
}

std::shared_ptr<const ISeq> LazySeq::next() const {
  std::shared_ptr<const ISeq> s = seq();
	if(s == NULL)
		return NULL;
	return s->next();
}

std::shared_ptr<const ISeq> LazySeq::more() const {
  std::shared_ptr<const ISeq> s = seq();
	if(s == NULL)
		return List::EMPTY;
	return s->next();
}

std::shared_ptr<const ICollection> LazySeq::cons(std::shared_ptr<const lisp_object> o) const {
  return RT::cons(o, seq());
}


std::shared_ptr<const lisp_object> LazySeq::sval() {
  if(fn) {
    sv = fn->invoke();
    fn = NULL;
  }
  if(sv)
    return sv;
  return s;
}


// Compiler.cpp

static const std::shared_ptr<const Symbol> DEF = Symbol::intern("def");
static const std::shared_ptr<const Symbol> LOOP = Symbol::intern("loop*");
static const std::shared_ptr<const Symbol> RECUR = Symbol::intern("recur");
static const std::shared_ptr<const Symbol> IF = Symbol::intern("if");
static const std::shared_ptr<const Symbol> LET = Symbol::intern("let*");
static const std::shared_ptr<const Symbol> LETFN = Symbol::intern("letfn*");
static const std::shared_ptr<const Symbol> DO = Symbol::intern("do");
static const std::shared_ptr<const Symbol> FN = Symbol::intern("fn*");


static std::shared_ptr<Var> LOCAL_ENV = Var::create(NULL)->setDynamic();
static std::shared_ptr<Var> METHOD = Var::create(NULL)->setDynamic();
static std::shared_ptr<Var> IN_CATCH_FINALLY = Var::create(NULL)->setDynamic();
static std::shared_ptr<Var> COMPILER_OPTIONS;

static std::shared_ptr<const lisp_object> getCompilerOption(std::shared_ptr<const Keyword> k){
	return RT::get(COMPILER_OPTIONS->deref(), k);
}

static std::shared_ptr<Var> LINE = Var::create(std::make_shared<Integer>(0))->setDynamic();
static std::shared_ptr<Var> COLUMN = Var::create(std::make_shared<Integer>(0))->setDynamic();
static long lineDeref(){
	return (long)*std::dynamic_pointer_cast<const Number>(LINE->deref());
}
static long columnDeref(){
	return (long)*std::dynamic_pointer_cast<const Number>(COLUMN->deref());
}

static std::shared_ptr<Var> CLEAR_PATH = Var::create(NULL)->setDynamic();
static std::shared_ptr<Var> CLEAR_ROOT = Var::create(NULL)->setDynamic();

static const std::shared_ptr<Keyword> disableLocalsClearingKey = Keyword::intern("disable-locals-clearing");


class CompilerException : public std::runtime_error {
  public:
    CompilerException(std::string source, long line, long column, std::runtime_error cause) :
          std::runtime_error(std::string(cause.what()) + ", compiling:(" + source + ":" + std::to_string(line) + ":" + std::to_string(column) + ")"),
          source(source), line(line) {};
    const std::string source;
  	const long line;
};

class UnsupportedOperationException : public std::runtime_error {
  public:
    UnsupportedOperationException(const std::string& what_arg) : std::runtime_error(what_arg) {};
};

typedef enum {
  STATEMENT,  //value ignored
	EXPRESSION, //value required
	RETURN,     //tail position relative to enclosing recur frame
	EVAL
} context;

typedef enum {
    PATH,
    BRANCH,
} PATHTYPE;

class PathNode{
  public:
    const PATHTYPE type;
    const std::shared_ptr<const PathNode> parent;

    PathNode(PATHTYPE type, std::shared_ptr<const PathNode> parent) :
      type(type), parent(parent) {};
};

static std::shared_ptr<const Symbol> tagOf(std::shared_ptr<const lisp_object> o){
	std::shared_ptr<const lisp_object> tag = RT::get(o->meta(), RT::TAG_KEY);
	std::shared_ptr<const Symbol> syTag = std::dynamic_pointer_cast<const Symbol>(tag);
	if(syTag)
	  return syTag;
	std::shared_ptr<const lisp_String> sTag = std::dynamic_pointer_cast<const lisp_String>(tag);
	if(sTag)
		return Symbol::intern("", sTag->toString());
	return NULL;
}

class ObjExpr;
class Expr {
  public:
    virtual std::shared_ptr<const lisp_object> eval() const = 0;
    // virtual void emit(context C, const ObjExpr &objx /*, GeneratorAdapter */) = 0;
    virtual bool hasClass() const = 0;
	  virtual const std::type_info* getClass() const = 0;
};

class LiteralExpr : public Expr {
  public:
    virtual std::shared_ptr<const lisp_object> val() const = 0;
    virtual std::shared_ptr<const lisp_object> eval() const {
      return val();
    };
};

class AssignableExpr {
  public:
  	virtual std::shared_ptr<const lisp_object> evalAssign(Expr& val) const = 0;
  	// virtual void emitAssign(context C, const ObjExpr &objx /*, GeneratorAdapter */) = 0;
};

class MaybePrimitiveExpr : public Expr{
  public:
	  virtual bool canEmitPrimitive() const = 0;
	  // void emitUnboxed(context C, const ObjExpr &objx /*, GeneratorAdapter */) = 0;
};

static const std::type_info* maybePrimitiveType(std::shared_ptr<const Expr> e) {
  std::shared_ptr<const MaybePrimitiveExpr> mpe = std::dynamic_pointer_cast<const MaybePrimitiveExpr>(e);
  if(mpe && mpe->hasClass() && mpe->canEmitPrimitive()) {
    const std::type_info* c = e->getClass();
    if(isPrimitive(c))
			return c;
  }
  return NULL;
}

class ObjMethod {
  public:
    ObjMethod(std::shared_ptr<ObjExpr> objx, std::shared_ptr<ObjMethod> parent) :
              objx(objx), parent(parent), localsUsedInCatchFinally(LSet::EMPTY),
              usesThis(false) {};

    bool usesThis;
    const std::shared_ptr<const IMap> locals;
    const std::shared_ptr<ObjExpr> objx;
    const std::shared_ptr<ObjMethod> parent;
    std::shared_ptr<const LSet> localsUsedInCatchFinally;
  private:
    virtual int numParams() = 0;
	  virtual std::string getMethodName() = 0;
};

class LocalBinding : public lisp_object {
  public:
    LocalBinding(int num, std::shared_ptr<const Symbol> sym, std::shared_ptr<const Symbol> tag,
                 std::shared_ptr<const Expr> init, bool isArg, std::shared_ptr<const PathNode> clearPathRoot) :
                 sym(sym), tag(tag), init(init), name(sym->getName()), isArg(isArg), clearPathRoot(clearPathRoot), 
                 canBeCleared(!((bool)getCompilerOption(disableLocalsClearingKey))), recurMistmatch(false),
                 used(false), idx(num) {};

    const std::shared_ptr<const Symbol> sym;
  	const std::shared_ptr<const Symbol> tag;
  	std::shared_ptr<const Expr> init;
  	const std::string name;
  	const bool isArg;
    const std::shared_ptr<const PathNode> clearPathRoot;
  	bool canBeCleared;
  	bool recurMistmatch;
    bool used;
    int idx;
    
    std::type_info* const getPrimitiveType() const {
      // TODO
    };
};

class NilExpr : public LiteralExpr {
  public:
    virtual std::shared_ptr<const lisp_object> val() const {
      return NULL;
    };
    virtual bool hasClass() const {return true;};
	  virtual const std::type_info* getClass() const {return NULL;};
};
static const std::shared_ptr<const NilExpr> NIL_EXPR = std::make_shared<const NilExpr>();

class BooleanExpr : public LiteralExpr {
  public:
    BooleanExpr(bool tf) : tf(tf) {};
    virtual std::shared_ptr<const lisp_object> val() const {
      return tf ? T : F;
    };
    virtual bool hasClass() const {return true;};
	  virtual const std::type_info* getClass() const {
	    return &typeid(Boolean);
	  };
  private:
    bool tf;
};
static const std::shared_ptr<const BooleanExpr> TRUE_EXPR = std::make_shared<const BooleanExpr>(true);
static const std::shared_ptr<const BooleanExpr> FALSE_EXPR = std::make_shared<const BooleanExpr>(false);

class ObjExpr : public Expr {
  public:
    std::shared_ptr<const IMap> closes;
    // TODO
};

class LocalBindingExpr : public MaybePrimitiveExpr, public AssignableExpr {
  public:
    LocalBindingExpr(std::shared_ptr<const LocalBinding> b, std::shared_ptr<const Symbol> tag);

    const std::shared_ptr<const LocalBinding> b;
	  const std::shared_ptr<const Symbol> tag;
    const std::shared_ptr<const PathNode> clearPath;
    const std::shared_ptr<const PathNode> clearRoot;
    bool shouldClear = false;

    virtual std::shared_ptr<const lisp_object> eval() const {
      throw UnsupportedOperationException("Can't eval locals");
    };
	  virtual std::shared_ptr<const lisp_object> evalAssign(Expr&) const {
      throw UnsupportedOperationException("Can't eval locals");
    };
    virtual bool hasClass() const {};   // TODO
	  virtual const std::type_info* getClass() const {};    // TODO
	  bool canEmitPrimitive() const {return b->getPrimitiveType();};
};

LocalBindingExpr::LocalBindingExpr(std::shared_ptr<const LocalBinding> b, std::shared_ptr<const Symbol> tag) :
                 b(b), tag(tag),
                 clearPath(std::dynamic_pointer_cast<const PathNode>(CLEAR_PATH->get())),
                 clearRoot(std::dynamic_pointer_cast<const PathNode>(CLEAR_ROOT->get())) {
  // TODO
};


static void closeOver(std::shared_ptr<const LocalBinding> b, std::shared_ptr<ObjMethod> method) {
  if(b && method) {
    std::shared_ptr<const LocalBinding> lb = std::dynamic_pointer_cast<const LocalBinding>(RT::get(method->locals, b));
    if(lb) {
      if(lb->idx == 0)
        method->usesThis = true;
        if(IN_CATCH_FINALLY->deref())
          method->localsUsedInCatchFinally = std::dynamic_pointer_cast<const LSet>(
                  method->localsUsedInCatchFinally->cons(std::make_shared<Integer>(b->idx)));
    } else {
      method->objx->closes = std::dynamic_pointer_cast<const IMap>(RT::assoc(method->objx->closes, b, b));
			closeOver(b, method->parent);
    }
  }
}

static std::shared_ptr<const LocalBinding> referenceLocal(std::shared_ptr<const Symbol> sym) {
  if(!LOCAL_ENV->isBound())
    return NULL;
  std::shared_ptr<const LocalBinding> b = std::dynamic_pointer_cast<const LocalBinding>(RT::get(LOCAL_ENV->deref(), sym));
  if(b) {
    std::shared_ptr<ObjMethod> method = std::const_pointer_cast<ObjMethod>(std::dynamic_pointer_cast<const ObjMethod>(METHOD->deref()));
    if(b->idx == 0)
      method->usesThis = true;
    closeOver(b, method);
  }
  return b;
}

std::shared_ptr<const lisp_object> macroexpand1(std::shared_ptr<const lisp_object> x) {
  // TODO
  return x;
}

static std::shared_ptr<const lisp_object> macroexpand(std::shared_ptr<const lisp_object> form) {
	std::shared_ptr<const lisp_object> exf = macroexpand1(form);
	if(exf != form)
		return macroexpand(exf);
	return form;
}

static std::shared_ptr<const Expr> analyzeSymbol(std::shared_ptr<const Symbol> sym) {
  std::shared_ptr<const Symbol> tag = tagOf(sym);
  if(sym->getNamespace() == "") {
    std::shared_ptr<const LocalBinding> b = referenceLocal(sym);
    if(b)
      return std::make_shared<const LocalBindingExpr>(b, tag);
  } else {
    // TODO
  }
  // TODO
}

static std::shared_ptr<const Expr> analyze(context c, std::shared_ptr<const lisp_object> form, std::string name) {
  try {
    std::shared_ptr<const LazySeq> mform = std::dynamic_pointer_cast<const LazySeq>(form);
    if(mform) {
      form = RT::seq(form);
      if(form == NULL)
        form = List::EMPTY;
        form = form->with_meta(mform->meta());
    }
    if(form == NULL)
      return NIL_EXPR;
    if(*form == *T)
      return TRUE_EXPR;
    if(*form == *F)
      return FALSE_EXPR;
    std::shared_ptr<const Symbol> sym = std::dynamic_pointer_cast<const Symbol>(form);
    if(sym)
      return analyzeSymbol(sym);
    // TODO
  } catch (const std::runtime_error &e) {
    try {
      const CompilerException &ce = dynamic_cast<const CompilerException&>(e);
      throw ce;
    } catch (const std::bad_cast&) {
      throw CompilerException("" /* TODO */, lineDeref(), columnDeref(), e);
    }
  }
}

static std::shared_ptr<const Expr> analyze(context c, std::shared_ptr<const lisp_object> form) {
  return analyze(c, form, "");
}


std::shared_ptr<const lisp_object> eval(std::shared_ptr<const lisp_object> form, bool freshLoader) {
	// Var.pushThreadBindings(RT.map(LOADER, RT.makeClassLoader()));  // TODO
	try {
	  long line = lineDeref();
		long column = columnDeref();
		if(form->meta() && form->meta()->containsKey(RT::LINE_KEY))
			line = (long)*std::dynamic_pointer_cast<const Number>(form->meta()->valAt(RT::LINE_KEY));
		if(form->meta() && form->meta()->containsKey(RT::COLUMN_KEY))
			column = (long)*std::dynamic_pointer_cast<const Number>(form->meta()->valAt(RT::COLUMN_KEY));
		// Var.pushThreadBindings(RT.map(LINE, line, COLUMN, column));  // TODO
		try {
		  form = macroexpand(form);
		  std::shared_ptr<const ISeq> s = std::dynamic_pointer_cast<const ISeq>(form);
		  if(s && (s->first() == DO)) {
		    for(s = s->next(); s->next() != NULL; s = s->next())
		      eval(s->first(), false);
		    return eval(s->first(), false);
		  }
		  std::shared_ptr<const IType> t = std::dynamic_pointer_cast<const IType>(form);
		  if(t) {
		    // TODO
		  }
		  std::shared_ptr<const ICollection> c = std::dynamic_pointer_cast<const ICollection>(form);
		  if(c) {
		    std::shared_ptr<const Symbol> sy = std::dynamic_pointer_cast<const Symbol>(RT::first(c));
		    if(sy->getName().substr(0,3) == "def") {
		      std::shared_ptr<const ObjExpr> fexpr = std::dynamic_pointer_cast<const ObjExpr>(analyze(EXPRESSION, RT::list(FN, LVector::EMPTY, form), "eval" + RT::nextID()));
		      std::shared_ptr<const IFn> fn = std::dynamic_pointer_cast<const IFn>(fexpr->eval());
		      return fn->invoke();
		    }
		  }
		  std::shared_ptr<const Expr> expr = analyze(EVAL, form);
		  return expr->eval();
		} catch(const std::exception &e) {
  	  // Var.popThreadBindings();   // TODO
  	  throw e;
  	}
	} catch(const std::exception &e) {
	  // Var.popThreadBindings();   // TODO
	  throw e;
	}
}

int main() {
  std::cout << "Hello World!\n";
}
















