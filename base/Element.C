
#include "Element.h"
#include <ctype.h>
#include <stdio.h>
#include <math.h>

namespace Rosegarden {

// Element protocol format:
// 
// (package::type (paramname intval) (paramname "string"))
//
// e.g. (sys::note (duration 1) (label "My Note (may contain brackets)") (pitch 128))

    Element::Element(const string &package, const string &type) :
	m_params(), m_package(package), m_type(type) { }

    Element::Element(const Element &e) // inefficient
    {
	parse(*this, e.flatten());
    }

    Element::Element(const string &s) throw (BadFormat) // inefficient
    {
	parse(*this, s);
    }

    Element &Element::operator=(const Element &e) // inefficient
    {
	if (this == &e) return *this;
	empty();
	parse(*this, e.flatten());
	return *this;
    }

    void Element::empty()
    {
	m_type = "NO TYPE";
	m_package = "NO PACKAGE";
	m_params = ParamMap();
    }

    void Element::parse(Element &e, const string &s)
	throw (BadFormat)
    {
	// not a very proper parser

	string::size_type i = 0, j = 0, k = 0;

	if ((i = nextParen(s, i, '(', false)) >= s.size()) {
	    throw BadFormat("Missing open parenthesis in \"" + s + "\"");
	}

	++i;

	string package = nextToken(s, i);
	i += package.size();

//	cout << "Package name: \"" << package << "\"" << endl;
	if (s[i++] != ':' || s[i++] != ':') {
	    throw BadFormat("Missing :: after package name in \"" + s + "\"");
	}
	e.setPackage(package);

	string type = nextToken(s, i);
	i += type.size();

//	cout << "Type name: \"" << type << "\"" << endl;
	if (type.size() == 0) {
	    throw BadFormat("Missing typename in \"" + s + "\"");
	}
	e.setType(type);

	while (i < s.size() && s[i] != ')') {

	    if (isspace(s[i])) { ++i; continue; }

	    if (s[i] == '(') {

		j = i + 1;

		i = nextParen(s, i, ')', true);
		if (i >= s.size()) {
		    throw BadFormat("Unmatched open-parenthesis in \"" + s + "\"");
		}

		string pname = nextToken(s, j);
		if (pname.size() == 0) {
		    throw BadFormat("Empty parameter name in \"" + s + "\"");
		}

//		cout << "Parameter name: \"" << pname << "\"" << endl;

		j += pname.size();
		while (isspace(s[j])) ++j;

		if (s[j] == ')') {
//		    cout << "It's a tag" << endl;
		    e.setTag(pname);
		} else if (s[j] == '"') {
		    k = j;
		    ++j;
		    while (s[++k] != '"');
		    string pval = s.substr(j, k-j);
//		    cout << "It's a string, value \"" << pval << "\"" << endl;
		    e.setString(pname, pval);
		} else if (isdigit(s[j])) {
		    k = j;
		    while (isdigit(s[k])) ++k;
		    string pvals = s.substr(j, k-j);
		    long pval = atol(pvals.c_str());
//		    cout << "It's an int, value " << pval << endl;
		    e.setInt(pname, pval);
//		    cout << "(Now the value of " << pname << " is " << e.getInt(pname) << ")" << endl;
		} else {
		    throw BadFormat("Syntax error after parameter name \"" +
				    pname + "\" in \"" + s + "\"");
		}
	    }

	    ++i;
	}	    

	if (i >= s.size()) {
	    throw BadFormat("Missing close parenthesis in \"" + s + "\"");
	}
    }

    size_t Element::getSize() const
    {
	size_t s = 4 + m_package.size() + m_type.size();

	ParamMap::const_iterator i = m_params.begin();
	while (i != m_params.end()) {

	    s += 2 + (*i).first.size();
	    const Param &p = (*i).second;

	    switch (p.type) {
	    case Param::Int:
		s += 1;
		s += (p.i <  0 ? (int)log10((double)-p.i) :
		      p.i == 0 ? 1 :
		      (int)log10((double) p.i));
		break;
	    case Param::String:
		s += 3 + p.s.size();
		break;
	    case Param::Tag:
		break;
	    }

	    ++i;
	}

	return s;
    }

    string Element::flatten() const
    {
	string s = "(" + m_package + "::" + m_type;

	ParamMap::const_iterator i = m_params.begin();
	while (i != m_params.end()) {

//	    cout << "Found " << (*i).first << ", " << (int)((*i).second.type) << endl;

	    s += " (" + (*i).first;
	    const Param &p = (*i).second;

	    switch (p.type) {
	    case Param::Int: {
		static char buffer[100];
		sprintf(buffer, " %ld", p.i);
		s += buffer;
		break;
	    }
	    case Param::String:
		s += " \"" + p.s + "\"";
		break;
	    case Param::Tag:
		break;
	    }

	    s += ")";
	    ++i;
	}

	s += ")";
	return s;
    }

    bool Element::hasInt(const string &pname) const throw (BadFormat)
    {
	ParamMap::const_iterator i = m_params.find(pname);
	if (i == m_params.end()) return false;
	else if ((*i).second.type == Param::Int) return true;
	else return badFormat(pname, "an int");
    }

    void Element::setInt(const string &pname, long val) throw (BadFormat)
    {
	ParamMap::iterator i = m_params.find(pname);
	if (i != m_params.end()) {
	    if ((*i).second.type != Param::Int) badFormat(pname, "an int");
	    else (*i).second.i = val;
	} else {
	    m_params.insert(ParamPair(pname, Param(val)));
	}
    }

    long Element::getInt(const string &pname) const throw (BadFormat, NoData)
    {
	ParamMap::const_iterator i = m_params.find(pname);
	if (i != m_params.end() && (*i).second.type == Param::Int) {
	    if ((*i).second.type != Param::Int) return badFormat(pname, "an int");
	    return (*i).second.i;
	}
	throw NoData();
    }

    bool Element::hasString(const string &pname) const throw (BadFormat)
    {
	ParamMap::const_iterator i = m_params.find(pname);
	if (i == m_params.end()) return false;
	else if ((*i).second.type == Param::String) return true;
	else return badFormat(pname, "a string");
    }

    void Element::setString(const string &pname, const string &val)
	throw (BadFormat)
    {
	ParamMap::iterator i = m_params.find(pname);
	if (i != m_params.end()) {
	    if ((*i).second.type != Param::String) badFormat(pname, "a string");
	    else (*i).second.s = val;
	} else {
	    m_params.insert(ParamPair(pname, Param(val)));
	}
    }

    string Element::getString(const string &pname) const
	throw (BadFormat, NoData)
    {
	ParamMap::const_iterator i = m_params.find(pname);
	if (i != m_params.end() && (*i).second.type == Param::String) {
	    if ((*i).second.type != Param::String) badFormat(pname, "a string");
	    return (*i).second.s;
	}
	throw NoData();
    }

    bool Element::hasTag(const string &pname) const throw (BadFormat)
    {
	ParamMap::const_iterator i = m_params.find(pname);
	if (i == m_params.end()) return false;
	else if ((*i).second.type == Param::Tag) return true;
	else return badFormat(pname, "a tag");
    }

    void Element::setTag(const string &pname) throw (BadFormat)
    {
	ParamMap::iterator i = m_params.find(pname);
	if (i != m_params.end()) {
	    if ((*i).second.type != Param::Tag) badFormat(pname, "a tag");
	    // already set, so leave it alone
	} else {
	    m_params.insert(ParamPair(pname, Param()));
	}
    }

    void Element::unsetTag(const string &pname) throw (BadFormat)
    {
	ParamMap::iterator i = m_params.find(pname);
	if (i != m_params.end()) {
	    if ((*i).second.type != Param::Tag) badFormat(pname, "a tag");
	    else m_params.erase(i);
	}
	// not set, so leave it alone
    }

    string::size_type Element::nextParen(string s, string::size_type i, char paren, bool skip)
    {
	if (!skip) {
	    while (i < s.size() && isspace(s[i])) ++i;
	} else {
	    while (i < s.size()) {
		if (s[i] == paren) break;
		if (s[i] == '"') {
		    ++i;
		    while (i < s.size() && s[i] != '"') ++i;
		}
		++i;
	    }
	}

	return ((i < s.size() && s[i] == paren) ? i : s.size());
    }

    string Element::nextToken(string s, string::size_type i)
    {
	while (i < s.size() && isspace(s[i])) ++i;
	string::size_type j = i;
	while (i < s.size() && (isalpha(s[i]) || s[i] == '-' ||
				(i > j && isdigit(s[i])))) ++i;
	if (i > j) return s.substr(j, i-j);
	else return "";
    }

};
    
