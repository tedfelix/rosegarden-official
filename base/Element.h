
#ifndef _ELEMENT_H_
#define _ELEMENT_H_

#include <map>
#include <string>

namespace Rosegarden {
	
    class Element
    {
    private:
	struct Param {
	    enum { Int, String, Tag } type;
	    
	    Param(long ii) : type(Int), i(ii) { }
	    Param(string ss) : type(String), s(ss) { }
	    Param() : type(Tag) { }
	    Param(const Param &p) : type(p.type), i(p.i), s(p.s) { }
	    Param &operator=(const Param &p) { i = p.i; s = p.s; return *this; }

	    // these can't all be in a union, as you can't have classes
	    // with constructors in unions
	    long i;
	    string s;
	};

	typedef map<string, Param> ParamMap;
	typedef ParamMap::value_type ParamPair;
	ParamMap m_params;

    public:
	struct BadFormat {
	    BadFormat(string r) : reason(r) { }
	    string reason;
	};
	struct NoData { };

	Element(const string &package, const string &type);
	Element(const string &) throw (BadFormat);
	Element(const Element &);
	virtual ~Element() { }

	Element &operator=(const Element &e);

	string getPackage() const { return m_package; }
	string getType() const { return m_type; }

	bool isPackage(const string &package) const {
	    return getPackage() == package;
	}
	bool isType(const string &package, const string &type) const {
	    return (isPackage(package) && getType() == type);
	}

	// All the has/set/get methods throw BadFormat if the parameter
	// you name exists but is not of the correct type.  If it doesn't
	// exist at all, the get methods throw NoData (so test the has
	// method first)
    
	bool hasInt(const string &pname) const throw (BadFormat);
	void setInt(const string &pname, long val) throw (BadFormat);
	long getInt(const string &pname) const throw (BadFormat, NoData);

	bool hasString(const string &pname) const throw (BadFormat);
	void setString(const string &pname, const string &val) throw (BadFormat);
	string getString(const string &pname) const throw (BadFormat, NoData);

	bool hasTag(const string &pname) const throw (BadFormat);
	void setTag(const string &pname) throw (BadFormat);
	void unsetTag(const string &pname) throw (BadFormat);

	size_t getSize() const;
	string flatten() const;

    private:
	string m_package;
	string m_type;

	static string::size_type nextParen(string, string::size_type, char, bool);
	static string nextToken(string, string::size_type);

	void setPackage(string p) { m_package = p; }
	void setType(string t) { m_type = t; }

	bool badFormat(const string &p, const string &t) const throw (BadFormat) {
	    throw BadFormat("Parameter " + p +
			    " exists, but is not " + t + " parameter");
	}

	void empty();
	static void parse(Element &, const string &) throw (BadFormat);
    };

};

#endif

    
