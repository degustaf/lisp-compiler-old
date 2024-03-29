

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
