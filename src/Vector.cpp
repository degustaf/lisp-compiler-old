#include <assert.h>
#include <cctype>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

class IMap;

class lisp_object : public std::enable_shared_from_this<lisp_object> {
  public:
    virtual std::string toString(void) {return ((lisp_object * const) this)->toString();};
    virtual std::string toString(void) const = 0;
    virtual std::shared_ptr<const lisp_object> with_meta(std::shared_ptr<const IMap>) const {return shared_from_this();};
    std::shared_ptr<const IMap> meta(void) const {return NULL;};
  protected:
    lisp_object(void) {};
    lisp_object(std::shared_ptr<const IMap>) {};
};
std::string toString(std::shared_ptr<const lisp_object>) {return "";};
class ISeq : virtual public lisp_object {
  public:
    virtual std::shared_ptr<const ISeq>next(void) const = 0;
    virtual std::shared_ptr<const lisp_object>first(void) const = 0;
    virtual size_t count(void) const {return 0;};
    virtual std::shared_ptr<const ISeq>cons(std::shared_ptr<const ISeq>, std::shared_ptr<const lisp_object>)const = 0;
};
class ILookup : virtual public lisp_object {
  public:
    // virtual std::shared_ptr<const lisp_object> valAt(std::shared_ptr<const lisp_object> key) const = 0;
    // virtual std::shared_ptr<const lisp_object> valAt(std::shared_ptr<const lisp_object> key,
    //                                                  std::shared_ptr<const lisp_object> NotFound) const = 0;
};
class Seqable : virtual public lisp_object {
  public :
    // virtual std::shared_ptr<const ISeq> seq() const = 0;
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
    // virtual std::shared_ptr<const Associative> assoc(std::shared_ptr<const lisp_object>,
    //                                                  std::shared_ptr<const lisp_object>) const = 0;
};
class IStack : virtual public ICollection {
  public:
    // virtual std::shared_ptr<const lisp_object>peek()const = 0;
    // virtual std::shared_ptr<const IStack>pop()const = 0;
};
class Indexed : virtual public Counted {
  public:
    // virtual std::shared_ptr<const lisp_object>nth(size_t i)const = 0;
    // virtual std::shared_ptr<const lisp_object>nth(size_t i, std::shared_ptr<const lisp_object> NotFound)const = 0;
};
class Reversible : virtual public lisp_object {
  public:
    // virtual std::shared_ptr<const lisp_object> rseq(void) const = 0;
};
class IMapEntry : virtual public lisp_object {
  public:
    virtual std::shared_ptr<const lisp_object>key(void) const = 0;
    virtual std::shared_ptr<const lisp_object>value(void) const = 0;
};
class IMap : virtual public lisp_object {
  public:
    virtual std::shared_ptr<const ISeq> seq(void) const = 0;
    virtual size_t count(void) const = 0;
};
class IVector : virtual public Associative, virtual public IStack, virtual public Reversible, virtual public Indexed {
  public:
    virtual size_t length (void) const = 0;
    // virtual std::shared_ptr<const IVector>assocN(size_t n, std::shared_ptr<const lisp_object> val) const = 0;
};

class ITransientCollection : virtual public lisp_object {
  public:
    // virtual std::shared_ptr<ITransientCollection> conj(std::shared_ptr<const lisp_object>) = 0;
    // virtual std::shared_ptr<const ICollection> persistent(void) = 0;
};
class ITransientAssociative : virtual public ITransientCollection, virtual public ILookup, virtual public Associative2 {
  public:
    // virtual std::shared_ptr<ITransientCollection> assoc(std::shared_ptr<const lisp_object>, std::shared_ptr<const lisp_object>) = 0;
};
class ITransientVector : virtual public ITransientAssociative, virtual public Indexed {
  public:
    // virtual std::shared_ptr<ITransientVector> assocN(size_t n, std::shared_ptr<const lisp_object> val) = 0;
    // virtual std::shared_ptr<ITransientVector> pop(void) = 0;
};

class Integer : virtual public lisp_object {
  public:
    Integer(long l) {};
    virtual std::string toString(void) const {return "";};
};
class Float : virtual public lisp_object {
  public:
    Float(double d) {};
    virtual std::string toString(void) const {return "";};
};

class ReaderError : public std::exception {
  public:
    ReaderError(size_t line, size_t column, std::exception e) : std::exception(e), line(line), column(column) {};
    
    const size_t line;
    const size_t column;
};
class NumberFormatException : public std::runtime_error {
  public:
    NumberFormatException(const std::string& buf) :
      std::runtime_error("Unable to convert '" + buf + "' into a number.") {};
};

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

class TransientVector;
class LVector : virtual public AVector {
  public:
    static std::shared_ptr<const LVector> create(std::vector<std::shared_ptr<const lisp_object> >);
    virtual std::shared_ptr<const ICollection>empty(void) const;
    virtual size_t count(void) const {return cnt;};
    std::shared_ptr<TransientVector> asTransient(void) const;

    static const std::shared_ptr<const LVector> EMPTY;
  private:
    const size_t cnt;
    const size_t shift;
    const std::shared_ptr<const Node> root;
    const std::vector<std::shared_ptr<const lisp_object> > tail;
    
    friend TransientVector;

    LVector(size_t cnt, size_t shift, std::shared_ptr<const Node> root, 
      const std::vector<std::shared_ptr<const lisp_object> > tail) : 
      cnt(cnt), shift(shift), root(root), tail(tail) {};
    LVector(std::shared_ptr<const IMap>meta, size_t cnt, size_t shift, std::shared_ptr<const Node> root, 
      const std::vector<std::shared_ptr<const lisp_object> > tail) : 
      lisp_object(meta), cnt(cnt), shift(shift), root(root), tail(tail) {};

};

class TransientVector : /* virtual public AFn,*/ virtual public ITransientVector, virtual public Counted {
  public:
    TransientVector(std::shared_ptr<const LVector> v): cnt(v->cnt), shift(v->shift),
      root(editableRoot(v->root)), tail(v->tail) {};
    TransientVector(size_t cnt, size_t shift, std::shared_ptr<Node> root, std::vector<std::shared_ptr<const lisp_object> > tail): cnt(cnt), shift(shift), root(root), tail(tail) {};
    virtual std::string toString(void) const { return "";};
    virtual size_t count(void) const {ensureEditable(); return cnt;};

  private:
    size_t cnt;
    size_t shift;
    std::shared_ptr<Node> root;
    std::vector<std::shared_ptr<const lisp_object> > tail;

    static std::shared_ptr<Node> editableRoot(const std::shared_ptr<const Node> node);
    static std::vector<std::shared_ptr<const lisp_object> > editableTail(std::vector<std::shared_ptr<const lisp_object> >);
    void ensureEditable(void) const;
};

std::shared_ptr<Node> TransientVector::editableRoot(const std::shared_ptr<const Node> node){
	return std::make_shared<Node>(std::this_thread::get_id(), node->array);
}

std::vector<std::shared_ptr<const lisp_object> > TransientVector::editableTail(std::vector<std::shared_ptr<const lisp_object> > tl) {
  std::vector<std::shared_ptr<const lisp_object> >ret = tl;
  ret.resize(NODE_SIZE);
	return ret;
}

void TransientVector::ensureEditable(void) const {
  if(root->id == std::hash<std::thread::id>{}(Node::NOEDIT))
    throw std::runtime_error("Transient used after persistent call");
}

const std::shared_ptr<const LVector> LVector::EMPTY(new LVector(0, LOG_NODE_SIZE, Node::EmptyNode, std::vector<std::shared_ptr<const lisp_object> >()));
std::shared_ptr<const ICollection> LVector::empty(void) const {
  return std::dynamic_pointer_cast<const ICollection>(LVector::EMPTY->with_meta(meta()));
};

std::shared_ptr<const LVector> LVector::create(std::vector<std::shared_ptr<const lisp_object> >list) {
  size_t size = list.size();
  if(size == 0)
    return EMPTY;
  if(size <= NODE_SIZE)
    return std::shared_ptr<const LVector>(new LVector(size, LOG_NODE_SIZE, Node::EmptyNode, list));
}

std::shared_ptr<TransientVector> LVector::asTransient(void) const {
  return std::make_shared<TransientVector>(std::dynamic_pointer_cast<const LVector>(shared_from_this()));
}



int main() {
  std::cout << "Hello World!\n";
}

















