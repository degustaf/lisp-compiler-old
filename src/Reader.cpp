std::shared_ptr<const lisp_object> read(LineNumberIStream *input, bool EOF_is_error, char return_on);

class ReaderError : public std::exception {
  public:
    ReaderError(size_t line, size_t column, std::exception e) : std::exception(e), line(line), column(column) {};
    
    const size_t line;
    const size_t column;
};

typedef std::shared_ptr<const lisp_object> (*MacroFn)(LineNumberIStream*, char /* *lisp_object opts, *lisp_object pendingForms */);



class ReaderConfig {
  public:
    ReaderConfig() : eof(ReaderSentinels("EOF")), done(ReaderSentinels("DONE")) {};
    class ReaderSentinels : public lisp_object {
      public:
        ReaderSentinels(std::string name) : name(name) {};
        virtual std::string toString(void) const {return name;};
        virtual std::shared_ptr<const IMap> meta(void) const {return NULL;};
      private:
        std::string name;
    };
    const ReaderSentinels eof;
    const ReaderSentinels done;
    
    MacroFn macros[128];
};

static const ReaderConfig config;

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
  	 
  	}
	}
	catch(std::exception e) {
	  if(isRecursive)
	    throw e;
	  throw new ReaderError(input.getLineNumber(), input.getColumnNumber(), e);
	}
}
