
#include "attribute_table.h"
#include <iostream>
#include <sstream>

namespace data {
	AttributeTable::AttributeTable() {
		//std::cout << "AttributeTable()" << std::endl;
	}

	AttributeTable::~AttributeTable() {
		//std::cout << "~AttributeTable()" << std::endl;
		clear();
	}

	data::AttributeTablePtr AttributeTable::create() {
		return data::AttributeTablePtr( new AttributeTable() );
	}

	std::map< std::string, std::vector<std::string> > AttributeTable::map_view() {
		return table_;
	}

	void AttributeTable::set( std::map< std::string, std::vector<std::string> > m ) {
		table_.clear();
		std::map< std::string, std::vector<std::string> >::const_iterator iter;
		for (iter = m.begin(); iter != m.end(); ++iter) {
			insert((*iter).first, (*iter).second);
		}
	}

	void AttributeTable::insert( const std::string& name, const std::vector<std::string> values ) {
		if ( name != "" ) {
			table_[ qualify(name) ] = values;
		}
	}

	void AttributeTable::insert( const std::string& name, const long value ) {
		if ( name.empty() ) {
			return;
		}
		std::vector<std::string> v;
		std::stringstream ss;
		ss << value;
		//std::cout << value << " " << ss.str() << std::endl;
		v.push_back( ss.str() );
		table_[ qualify(name) ] = v;
	}
	void AttributeTable::insert( const std::string& name, std::vector<long> values ) {
		if ( name.empty() ) {
			return;
		}
		std::vector<std::string> v;
		std::stringstream ss;
		for ( int i = 0; i < values.size(); i++ ) {
			ss.str( "" );
			ss << values[i];
			//std::cout << value << " " << ss.str() << std::endl;
			v.push_back( ss.str() );
		}
		table_[ qualify(name) ] = v;
	}
	void AttributeTable::insert( const std::string& name, const std::string value ) {
		if ( name.empty() ) {
			return;
		} 
		std::vector<std::string> values;
		values.push_back( value );
		table_[ qualify(name) ] = values;

	}

	void AttributeTable::merge( data::AttributeTablePtr source ) {
		if ( !source ) return;
		std::map< std::string, std::vector<std::string> > smap = source->map_view();
		std::map< std::string, std::vector<std::string> >::iterator first = smap.begin();
		std::map< std::string, std::vector<std::string> >::iterator last = smap.end();
		table_.insert(first, last);
	}
	std::string AttributeTable::qualify(const std::string& name) const {
		if ( name.find(".") == 0 ) {
			return prefix_ + name;
		}
		if ( name.find(".") == std::string::npos ) {
			return prefix_ + "." + name;
		}
		return name;
	}

	void AttributeTable::display_attributes() const {
		std::map< std::string, std::vector<std::string> >::const_iterator iter;
		for (iter = table_.begin(); iter != table_.end(); ++iter) {
			std::cout << "\t[+] " << iter->first << std::endl;;
			std::string separator = ",";
			std::string str = "";
			std::vector<std::string>::const_iterator inn_iter;
			std::cout << "\t\t";
			for (inn_iter = ( iter->second ).begin(); inn_iter != (iter->second).end(); ++inn_iter) {
				str += *inn_iter;
				str += separator;
			}
			if ( str.length() > 0 ) {
				str.erase( str.length() - separator.length() );
			}
			std::cout << str << std::endl;
		}
	}

	void AttributeTable::clear() {
		table_.clear();
	}

	size_t AttributeTable::size() const {
		return table_.size();
	}
	bool AttributeTable::empty() const {
		return table_.empty();
	}

	bool AttributeTable::contains( std::string name ) const {
		//if ( table_.empty() ) {
		//	return false;
		//}
		if ( table_.count( qualify(name) ) ) {
			return true;
		}
		return false;
	}
	std::string AttributeTable::get_string( std::string name ) const {
		std::string n = "";
		if ( contains(name) ) {
			std::vector<std::string> r = get_string_array( name );
			
			if ( r.size() ) {
				return r[0];
			}
		}
		return n;

	}
	std::vector<std::string> AttributeTable::get_string_array( std::string name ) const {
		std::vector<std::string> n;
		if ( contains(name) ) {
			std::vector<std::string> r = table_.find( qualify(name) )->second;
			return r;
		}
		return n;
	}

	long AttributeTable::get_long( std::string name ) const {
		long l = 0;
		if ( contains(name) ) {
			std::string s = get_string( name );
			//std::cout << s << std::endl;
			std::stringstream ss( s );
			ss >> l;
		}
		return l;
	}

	std::vector<long> AttributeTable::get_long_array( std::string name ) const {
		std::vector< long > v;
		if ( contains(name) ) {
			std::vector< std::string > sv = get_string_array( name );
			for ( int i = 0; i < sv.size(); i++ ) {
				std::stringstream ss( sv[i] );
				long l = 0;
				ss >> l;
				//std::cout << ss.str() << " ";
				v.push_back( l );
			}
			//std::cout << std::endl;
		}
		return v;
	}

	double AttributeTable::get_double( std::string name ) const {
		double d = 0;
		if ( contains(name) ) {
			std::string s = get_string( name );
			//std::cout << s << std::endl;
			std::stringstream ss( s );
			ss >> d;
		}
		return d;
	}
	std::vector<double> AttributeTable::get_double_array( std::string name ) const {
		std::vector<double> v;
		if ( contains(name) ) {
			std::vector< std::string > sv = get_string_array( name );
			for ( int i = 0; i < sv.size(); i++ ) {
				std::stringstream ss( sv[i] );
				double l = 0.0;
				ss >> l;
				//std::cout << ss.str() << " ";
				v.push_back( l );
			}
			//std::cout << std::endl;
		}
		return v;
	}
}
