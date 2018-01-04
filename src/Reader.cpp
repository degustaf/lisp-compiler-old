#include <assert.h>
#include <cctype>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

class IMap;

class lisp_object {
  public:
    virtual std::string toString(void) {return ((lisp_object * const) this)->toString();};
    virtual std::string toString(void) const = 0;
    std::shared_ptr<const IMap> meta(void) const {return NULL;};
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

class ISeq : virtual public lisp_object {
  public:
    virtual std::shared_ptr<const ISeq>next(void) const = 0;
    virtual std::shared_ptr<const lisp_object>first(void) const = 0;
    virtual size_t count(void) const {return 0;};
    virtual std::shared_ptr<const ISeq>cons(std::shared_ptr<const ISeq>, std::shared_ptr<const lisp_object>)const = 0;
};

class List : virtual public ISeq {
  public:
    List(std::shared_ptr<const lisp_object> first) : _first(first), _rest(NULL), _count(1) {};
    List(std::shared_ptr<const IMap> meta, std::shared_ptr<const lisp_object> first, std::shared_ptr<const ISeq> rest, size_t count) : _first(first), _rest(rest), _count(count) {};
    List(std::vector<std::shared_ptr<const lisp_object> >& entries) : _first(entries[0]), _rest(createRest(entries)), _count(entries.size()) {};
    virtual std::string toString(void) const {return "";};
    virtual std::shared_ptr<const ISeq>next(void) const {return NULL;};
    virtual std::shared_ptr<const lisp_object>first(void) const {return NULL;};
    virtual std::shared_ptr<const ISeq>cons(std::shared_ptr<const ISeq>, std::shared_ptr<const lisp_object>)const ;
    //static const std::shared_ptr<const ISeq> Empty = NULL;
  private:
    static std::shared_ptr<const ISeq>createRest(std::vector<std::shared_ptr<const lisp_object> >& entries);
    std::shared_ptr<const lisp_object> _first;
    std::shared_ptr<const ISeq> _rest;
    const size_t _count;
    
};

std::shared_ptr<const ISeq>List::cons(std::shared_ptr<const ISeq>rest, std::shared_ptr<const lisp_object> first) const {
  return std::make_shared<List>(rest->meta(), first, rest, rest->count()+1);
}

std::shared_ptr<const ISeq>List::createRest(std::vector<std::shared_ptr<const lisp_object> >& entries) {
  size_t i = entries.size() - 1;
  std::shared_ptr<const ISeq> ret = std::make_shared<List>(entries[i]);
  for(i--; i > 0; i--)
    ret = ret->cons(ret, entries[i]);
  return ret;
}

class IMap : virtual public lisp_object {
  public:
    virtual std::shared_ptr<const ISeq> seq(void) const = 0;
    virtual size_t count(void) const = 0;
};

class MapEntry : virtual public lisp_object {
  public:
    const std::shared_ptr<const lisp_object> key;
    const std::shared_ptr<const lisp_object> val;
};

class IVector : virtual public lisp_object {
  public:
    virtual size_t count(void) const = 0;
    virtual std::shared_ptr<const lisp_object>nth(size_t n) const = 0;
};

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

size_t LineNumberIStream::getLineNumber(void) const {
  return line;
}

size_t LineNumberIStream::getColumnNumber(void) const {
  return column;
}

int LineNumberIStream::get(void) {
  int ret = is.get();
	prevAtLineStart = atLineStart;

	if(ret == '\n' || ret == EOF) {
		if(line) {
			line++;
			column = 1;
		}
		atLineStart = true;
		return ret;
	}

	if(column) column++;
	atLineStart = false;
	return ret;
}

void LineNumberIStream::unget(int ch) {
	if(atLineStart && line)
		line--;
	else if(column)
		column--;
	atLineStart = prevAtLineStart;

	is.putback(ch);
}



class Char : public lisp_object {
  public:
    Char(char c) : s(&c, 1) {};
    virtual std::string toString(void) const {return s;};
  private:
    const std::string s;
};

class lisp_string : public lisp_object {
  public:
    lisp_string(const std::string& s) : s(s) {};
    virtual std::string toString(void) const {return s;};
  private:
    const std::string s;
};

class lisp_bool : public lisp_object {
  public:
    lisp_bool(bool tf) {};
    virtual std::string toString(void) const {return "";};
};
static const lisp_bool T(true);
static const lisp_bool F(false);

std::shared_ptr<const lisp_object> read(LineNumberIStream *input, bool EOF_is_error, char return_on);

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

//typedef std::shared_ptr<const lisp_object> (*MacroFn)(LineNumberIStream&, char /* *lisp_object opts, *lisp_object pendingForms */);
typedef std::function<std::shared_ptr<const lisp_object>(LineNumberIStream& /* *lisp_object opts, *lisp_object pendingForms */)> MacroFn;

static std::shared_ptr<const lisp_object> CharReader(LineNumberIStream& input /* *lisp_object opts, *lisp_object pendingForms */);
static std::shared_ptr<const lisp_object> ListReader(LineNumberIStream& input /* *lisp_object opts, *lisp_object pendingForms */);
static std::shared_ptr<const lisp_object> VectorReader(LineNumberIStream& input /* *lisp_object opts, *lisp_object pendingForms */);
static std::shared_ptr<const lisp_object> MapReader(LineNumberIStream& input /* *lisp_object opts, *lisp_object pendingForms */);
static std::shared_ptr<const lisp_object> StringReader(LineNumberIStream& input /* *lisp_object opts, *lisp_object pendingForms */);
static std::shared_ptr<const lisp_object> UnmatchedParenReader(LineNumberIStream& input, char ch /* *lisp_object opts, *lisp_object pendingForms */);
static std::shared_ptr<const lisp_object> WrappingReader(LineNumberIStream& input, char ch /* *lisp_object opts, *lisp_object pendingForms */);
static std::shared_ptr<const lisp_object> CommentReader(LineNumberIStream& input /* *lisp_object opts, *lisp_object pendingForms */);
static std::shared_ptr<const lisp_object> MetaReader(LineNumberIStream& input /* *lisp_object opts, *lisp_object pendingForms */);

class ReaderConfig {
  public:
    ReaderConfig() : eof(ReaderSentinels("EOF")), done(ReaderSentinels("DONE")), noop(ReaderSentinels("NOOP")) {
      macros['\\'] = CharReader;
    	macros['"']  = StringReader;
    	macros['(']  = ListReader;
    	macros[')']  = std::bind(UnmatchedParenReader, std::placeholders::_1, ')');
    	// macros['[']  = VectorReader;
    	macros[']']  = std::bind(UnmatchedParenReader, std::placeholders::_1, ']');
    	// macros['{']  = MapReader;
    	macros['}']  = std::bind(UnmatchedParenReader, std::placeholders::_1, '}');
    	// macros['\''] = WrappingReader;
    	// macros['@']  = WrappingReader;
    	macros[';']  = CommentReader;
    	// macros['^'] = MetaReader;
    
    	// macros['`'] = new SyntaxQuoteReader();
    	// macros['~'] = new UnquoteReader();
    	// macros['%'] = new ArgReader();
    	// macros['#'] = new DispatchReader();
    };
    class ReaderSentinels : public lisp_object {
      public:
        ReaderSentinels(std::string name) : name(name) {};
        virtual std::string toString(void) const {return name;};
      private:
        std::string name;
    };
    const ReaderSentinels eof;
    const ReaderSentinels done;
    const ReaderSentinels noop;
    
    MacroFn macros[128];
};

static const ReaderConfig config;

static bool isMacro(int ch) {
  return ((size_t)ch < sizeof(config.macros)/sizeof(*config.macros)) && (config.macros[ch] != NULL);
}

static bool isTerminatingMacro(int ch) {
	return ch != '#' && ch != '\'' && ch != '%' && isMacro(ch);
}

static MacroFn get_macro(int ch) {
	if((size_t)ch < sizeof(config.macros)/sizeof(*config.macros)) {
		return config.macros[ch];
	}
	return NULL;
}

std::shared_ptr<const lisp_object> ReadNumber(LineNumberIStream &input, char ch) {
	std::ostringstream result;
	while(ch != EOF && !isspace(ch) && !isMacro(ch)) {
	  result << ch;
	  ch = input.get();
	}
	input.unget(ch);
	
	std::istringstream number(result.str());
	long l;
	number >> l;
	if(number.eof())
		return std::make_shared<Integer>(l);

	number = std::istringstream(result.str());
	double d;
	number >> d;
	if(number.eof())
		return std::make_shared<Float>(d);
	
	throw NumberFormatException(result.str());
}

static std::string ReadToken(LineNumberIStream &input, char ch) {
  std::ostringstream result;
	while(ch != EOF && !isspace(ch) && !isMacro(ch)) {
	  result << ch;
	  ch = input.get();
	}
	input.unget(ch);

  return result.str();
}

static std::shared_ptr<const lisp_object> matchSymbol(std::string& token) {
  // TODO
  return NULL;
}

static std::shared_ptr<const lisp_object> interpretToken(std::string& token) {
	if("nil" == token)
		return NULL;
	if("true" == token)
		return std::make_shared<lisp_bool>(T);
	if("false" == token)
		return std::make_shared<lisp_bool>(F);

	const std::shared_ptr<const lisp_object> ret = matchSymbol(token);
	if(ret)
		return ret;

  throw std::runtime_error("Invalid token: " + token);
}

std::shared_ptr<const lisp_object> read(LineNumberIStream &input, bool EOF_is_error, char return_on, bool isRecursive /*, *lisp_object opts, *lisp_object pendingForms */) {
  try {
  	int ch = input.get();
  	
  	while(true) {
      while(isspace(ch))
  			ch = input.get();
  
      if(ch == EOF) {
  			if(EOF_is_error) {
  				throw std::runtime_error("EOF while reading");
  			}
  			return std::shared_ptr<const lisp_object>(&config.eof);
  	  }
  	  
  	  if(ch == return_on)
  	    return std::shared_ptr<const lisp_object>(&config.done);
  	 
      MacroFn macro = get_macro(ch);
  		if(macro) {
  			std::shared_ptr<const lisp_object> ret = macro(input);
  			if(ret.get() == &config.noop) {
  				continue;
  			}
  			return ret;
  		}
  		
  		if(ch == '+' || ch == '-') {
		  	int ch2 = input.get();
  			if(isdigit(ch2)) {
	  			input.unget(ch2);
				  return ReadNumber(input, ch);
			  }
		  }
		  
		  if(isdigit(ch))
			  return ReadNumber(input, ch);

      std::string token = ReadToken(input, ch);
  		std::shared_ptr<const lisp_object> ret = interpretToken(token);
  		return ret;
  	}
	}
	catch(std::exception e) {
	  if(isRecursive)
	    throw e;
	  throw new ReaderError(input.getLineNumber(), input.getColumnNumber(), e);
	}
}

static std::shared_ptr<const lisp_object> CharReader(LineNumberIStream& input /* *lisp_object opts, *lisp_object      pendingForms */) {
  char ch = input.get();
	if(ch == EOF)
		throw std::runtime_error("EOF while reading character");

	std::string token = ReadToken(input, ch);
	if(token.size() == 1)
		return std::make_shared<Char>(token[0]);
	if(token == "newline")
		return std::make_shared<Char>('\n');
	if(token == "space")
		return std::make_shared<Char>(' ');
	if(token == "tab")
		return std::make_shared<Char>('\t');
	if(token == "backspace")
		return std::make_shared<Char>('\b');
	if(token == "formfeed")
		return std::make_shared<Char>('\f');
	if(token, "return")
		return std::make_shared<Char>('\r');

  throw std::runtime_error("Unsupported character: \\" + token);
}

static std::shared_ptr<const lisp_object> StringReader(LineNumberIStream& input /* *lisp_object opts, *lisp_object      pendingForms */) {
  std::ostringstream ret;
  for(int ch = input.get(); ch != '"'; ch = input.get()) {
    if(ch == EOF)
      throw std::runtime_error("EOF while reading string.");
    if(ch == '\\') {
      ch = input.get();
      if(ch == EOF)
        throw std::runtime_error("EOF while reading string.");

        switch(ch) {
  				case 't':
  					ch = '\t';
  					break;
  				case 'r':
  					ch = '\r';
  					break;
  				case 'n':
  					ch = '\n';
  					break;
  				case 'b':
  					ch = '\b';
  					break;
  				case 'f':
  					ch = '\f';
  					break;
  				case '\\':
  				case '"':
  					break;
  				default:
  				  throw std::runtime_error("Unsupported Escape character: \\" + ch);
        }
    }
    ret << ch;
  }
  return std::make_shared<lisp_string>(ret.str());
}

static std::vector<std::shared_ptr<const lisp_object> > ReadDelimitedList(LineNumberIStream& input, char delim /* *lisp_object opts, *lisp_object pendingForms */) {
	const size_t firstLine = input.getLineNumber();
	std::vector<std::shared_ptr<const lisp_object> > ret;

	while(true) {
		std::shared_ptr<const lisp_object> form = read(input, true, delim, true);
		if(form.get() == &config.eof) {
		  if(firstLine > 0)
		    throw std::runtime_error("EOF while reading, starting at line "+ firstLine);
		  else
		    throw std::runtime_error("EOF while reading");
		}
		if(form.get() == &config.done)
			return ret;
		ret.push_back(form);
	}
}

static std::shared_ptr<const lisp_object> ListReader(LineNumberIStream& input /* *lisp_object opts, *lisp_object pendingForms */) {
	size_t line = input.getLineNumber();
	size_t column = line ? input.getColumnNumber() - 1 : 0;
	size_t count;
	std::vector<std::shared_ptr<const lisp_object> > list = ReadDelimitedList(input, ')');
	// if(count == 0) return List::Empty;
	std::shared_ptr<const lisp_object> s = std::make_shared<List>(list);
	// if(line) {
	// 	const lisp_object *margs[] = {
	// 		(lisp_object*) LineKW, (lisp_object*) NewInteger(line),
	// 		(lisp_object*) ColumnKW, (lisp_object*) NewInteger(column),
	// 	};
	// 	size_t margc = sizeof(margs) / sizeof(margs[0]);
	//	return withMeta(s, (IMap*) CreateHashMap(margc, margs));
	// }
	return s;
}

static std::shared_ptr<const lisp_object> UnmatchedParenReader(LineNumberIStream& input, char ch /* *lisp_object opts, *lisp_object pendingForms */) {
  throw std::runtime_error(std::string("Unmatched delimiter: ") + ch);
}

static std::shared_ptr<const lisp_object> CommentReader(LineNumberIStream& input /* *lisp_object opts, *lisp_object pendingForms */) {
	for(int ch = 0; ch != EOF && ch != '\n' && ch != '\r'; ch = input.get()) {}
	return std::shared_ptr<const lisp_object>(&config.noop);
}
