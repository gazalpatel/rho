#ifndef SYMBOL_SERIALIZATION_HPP
#define SYMBOL_SERIALIZATION_HPP

#include <string>

namespace CXXR {
    class GCNode;
    class RObject;
    class Symbol;
    // Forward declarations, defined in .cpp
    GCNode* composeSymbol(std::string&);
    const char* decomposeSymbol(Symbol*);

    template<class Archive>
    void saveSymbol(Archive & ar, const GCNode* pcs) {
	std::string sym(decomposeSymbol(const_cast<Symbol*>(static_cast<const Symbol*>(pcs))));
	ar << sym;
    }

    template<class Archive>
    GCNode* loadSymbol(Archive & ar) {
	std::string tmp;
	ar >> tmp;
	return composeSymbol(tmp);
    }
}

#endif
